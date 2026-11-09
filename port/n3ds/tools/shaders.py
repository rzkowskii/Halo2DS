"""Fresh PICA headers and linked-byte validation for incremental native builds."""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess


UV_OUTPUT_PROGRAMS = {
    'rigid_lit': 'texcoord',
    'skin_lit': 'texcoord',
    'rigid_reflect': 'maps',
    'skin_reflect': 'maps',
}
BASIC_UV_PROGRAMS = {'rigid', 'skin'}
VIEW_PACK_PROGRAMS = {'rigid_reflect', 'skin_reflect'}
CHICAGO_TRANSFORM_PROGRAMS = {'rigid_chicago', 'skin_chicago'}
HOMOGENEOUS_MODEL_PROGRAMS = (BASIC_UV_PROGRAMS | set(UV_OUTPUT_PROGRAMS) |
                              CHICAGO_TRANSFORM_PROGRAMS | {'rigid_depth', 'skin_depth'})
HOMOGENEOUS_EFFECT_PROGRAMS = {'effect_packed', 'chicago', 'hologram', 'glass', 'water'}
BOOLEAN_PROGRAMS = {'rigid_reflect', 'skin_reflect', 'water', 'glass'}
SKIN_MAD_PROGRAMS = {
    'skin': (1, 2), 'skin_lit': (1, 2), 'skin_reflect': (1, 2),
    'skin_depth': (1,), 'skin_chicago': (1,),
}


def _replace_once(source, old, new, description):
    if source.count(old) != 1:
        raise ValueError(f'Expected one {description} sequence')
    return source.replace(old, new)


def _uv_output_variant(source, target):
    # Both halves write the same products of the x/y lanes. Match the exact
    # pair so independently scaled material outputs cannot enter this slice.
    original = f'    mul {target}.xy, options, v2\n    mul {target}.zw, options.xyxy, v2.xyxy\n'
    combined = f'    mul {target}, options.xyxy, v2.xyxy\n'
    return _replace_once(source, original, combined, f'UV output instruction pair for {target}')


def _basic_model_uv_variant(source):
    # The stock texcoord0/1 output types export only XY. Neither declares
    # TEXCOORD0W, so the two identical pairs fit in one output register.
    source = _replace_once(source, '.out texcoord texcoord0\n.out neutral1 texcoord1\n',
                           '.out texcoord texcoord0.xy\n.out - texcoord1 texcoord.zw\n',
                           'basic model UV output declarations')
    return _replace_once(source,
                         '    mul texcoord.xy, options, v2\n    mov texcoord.zw, constants\n'
                         '    mul neutral1.xy, options, v2\n    mov neutral1.zw, constants\n',
                         '    mul texcoord, options.xyxy, v2.xyxy\n', 'basic model UV calculations')


def _model_view_variant(source):
    if source.count('.out view view\n') != 1:
        raise ValueError('Expected one three-component view export')
    # libctru exports only XYZ for RESULT_VIEW. All remaining uses of r4
    # are three-component, so the initialized fourth lane is a dummy.
    return _replace_once(source,
                         '    add r4.xyz, camera, -r1\n    mov view.xyz, r4\n    mov view.w, constants.x\n',
                         '    add r4, camera, -r1\n    mov view, r4\n', 'model view output')


def _model_quaternion_variant(source):
    # Finish the independent UV outputs first. Both exits then write the
    # quaternion once, with the original product or the original fallback.
    uv = '    mul maps.xy, options, v2\n    mul maps.zw, options.xyxy, v2.xyxy\n'
    setup = ('    mov r6.xy, r2\n    add r6.z, constants.y, r2.z\n'
             '    mov r6.w, constants.x\n    dp3 r3.w, r6, r6\n')
    compare = '    cmp constants.w, ge, ge, r3.w\n'
    original = (setup + '    mov r9, constants.yxxx\n' + compare +
                '    jmpc cmp.x, done_normal\n    rsq r3.w, r3.w\n'
                '    mul r9, r6, r3.w\ndone_normal:\n    mov quaternion, r9\n' +
                uv + '    end\n.end\n')
    candidate = (uv + setup + compare +
                 '    jmpc cmp.x, fallback_normal\n    rsq r3.w, r3.w\n'
                 '    mul quaternion, r6, r3.w\n    end\nfallback_normal:\n'
                 '    mov quaternion, constants.yxxx\n    end\n.end\n')
    return _replace_once(source, original, candidate, 'model normal quaternion tail')


def _model_reflection_variant(source):
    source = _replace_once(source, '.proc main\n',
                           '.alias reflection_enabled b0\n.proc main\n', 'model reflection selector')
    defaults = ('    mov r7, constants.yyyy\n    mov r8, constants.xxyx\n'
                '    ; With reflection disabled, its delta register carries independent detail UV scale.\n'
                '    mul r8.xy, reflection_delta, v2\n    mov r3.y, options.w\n'
                '    cmp constants.x, ge, ge, r3.y\n    jmpc cmp.x, diffuse_only\n')
    source = _replace_once(source, defaults, '    ifu reflection_enabled\n', 'reflection dispatch')
    source = _replace_once(source, '    add r8.xyz, r5, -r4\n',
                           '    add cube.xyz, r5, -r4\n', 'reflection coordinates')
    return _replace_once(source,
                         '    add r7, reflection_parallel, r5\ndiffuse_only:\n'
                         '    mov color, r7\n    mov cube, r8\n',
                         '    add color, reflection_parallel, r5\n'
                         '    .else\n    mov color, constants.yyyy\n'
                         '    mul cube.xy, reflection_delta, v2\n    mov cube.z, constants.y\n    .end\n',
                         'reflection outputs')


def _homogeneous_dots(source, destination, uniform, old_input, new_input, lanes):
    for row, lane in enumerate(lanes):
        source = _replace_once(source, f'    dp4 {destination}.{lane}, {uniform}[{row}], {old_input}\n',
                               f'    dph {destination}.{lane}, {new_input}, {uniform}[{row}]\n',
                               f'{destination} dot product')
    return source


def _model_homogeneous_variant(source, name):
    # DPH substitutes 1 for the first source's W. Keep the three spatial
    # products, translation, dot-product order and palette addresses intact.
    one = 'y' if name in UV_OUTPUT_PROGRAMS else 'w'
    source = _replace_once(source, f'    mov r0.xyz, v0\n    mov r0.w, constants.{one}\n',
                           '', 'model position setup')
    source = _replace_once(source, f'    mov r1.w, constants.{one}\n', '', 'model projection setup')
    if name.startswith('skin'):
        transforms = [('r1', 'a0.x+'), ('r4', 'a0.y+')]
    else:
        transforms = [('r1', '')]
    for result, index in transforms:
        for row, lane in enumerate('xyz'):
            uniform = f'bone[{index}{row}]'
            source = _replace_once(source, f'    dp4 {result}.{lane}, {uniform}, r0\n',
                                   f'    dph {result}.{lane}, v0, {uniform}\n', 'model bone dot product')
    source = _homogeneous_dots(source, 'position', 'projection', 'r1', 'r1', 'xyzw')
    # The optional packed view computes a dummy fourth lane. Swizzle it
    # from initialized XYZ now that the model position no longer sets W.
    if '    add r4, camera, -r1\n' in source:
        source = _replace_once(source, '    add r4, camera, -r1\n',
                               '    add r4, camera, -r1.xyzz\n', 'packed model view input')
    return source


def _effect_homogeneous_variant(source, name):
    if name == 'glass':
        # The posed and unposed paths share r0.xyz. Only the explicit W goes.
        source = _replace_once(source, '    mov r0.w, constants.y\n', '', 'glass position W')
        source = _homogeneous_dots(source, 'r6', 'pose', 'r0', 'r0', 'xyz')
        return _homogeneous_dots(source, 'position', 'projection', 'r0', 'r0', 'xyzw')
    one = 'y' if name == 'water' else 'w'
    source = _replace_once(source, f'    mov r0.xyz, v0\n    mov r0.w, constants.{one}\n',
                           '', 'effect position setup')
    source = _homogeneous_dots(source, 'position', 'projection', 'r0', 'v0', 'xyzw')
    if name == 'hologram':
        source = _homogeneous_dots(source, 'uv0', 'screen_rows', 'r0', 'v0', 'xyz')
    if name == 'water':
        source = _replace_once(source, '    add r3.xyz, camera, -r0\n',
                               '    add r3.xyz, camera, -v0\n', 'water eye position')
    return source


def _chicago_map_uv_variant(source):
    # The transform programs export XY only. The separate CPU-vertex program
    # exports TEXCOORD0W for projective effects and must keep its own path.
    source = _replace_once(source, '.proc main\n',
                           '.alias second_map b0\n.alias third_map b1\n.proc main\n',
                           'Chicago map selectors')
    rows = [f'    dp4 uv{i}.x, uv_rows[{2*i}], r0\n'
            f'    dp4 uv{i}.y, uv_rows[{2*i+1}], r0\n' for i in range(3)]
    original = ''.join(f'{row}    mov uv{i}.zw, constants\n' for i, row in enumerate(rows))
    candidate = (rows[0] + '    jmpu !second_map, no_second_map\n' + rows[1] +
                 '    jmpu !third_map, no_third_map\n' + rows[2] + '    end\n'
                 'no_second_map:\n    mov uv1.xy, constants\n'
                 'no_third_map:\n    mov uv2.xy, constants\n')
    return _replace_once(source, original, candidate, 'Chicago map UV tail')


def _boolean_variant(source, name):
    if name in {'rigid_reflect', 'skin_reflect'}:
        source = _replace_once(source, '.proc main\n',
                               '.alias reflection_enabled b0\n.proc main\n', 'main entry')
        return _replace_once(source,
                             '    mov r3.y, options.w\n    cmp constants.x, ge, ge, r3.y\n'
                             '    jmpc cmp.x, diffuse_only\n',
                             '    jmpu !reflection_enabled, diffuse_only\n', 'model reflection selector')
    if name == 'water':
        source = _replace_once(source, '.fvec wave_switch\n',
                               '.alias waves_enabled b0\n', 'water wave uniform')
        return _replace_once(source,
                             '    mov r1.w, wave_switch.x\n    cmp constants.x, ge, ge, r1.w\n'
                             '    jmpc cmp.x, wave_ready\n',
                             '    jmpu !waves_enabled, wave_ready\n', 'water wave selector')
    source = _replace_once(source, '.proc main\n',
                           '.alias rigid_enabled b0\n.alias diffuse_enabled b1\n.proc main\n', 'main entry')
    source = _replace_once(source,
                           '    mov r1.w, options.y\n    cmp constants.x, lt, lt, r1.w\n'
                           '    jmpc cmp.x, rigid_pose\n    cmp r1.w, eq, eq, r1.w\n'
                           '    jmpc cmp.x, pose_ready\nrigid_pose:\n',
                           '    jmpu !rigid_enabled, pose_ready\n', 'glass rigid pose selector')
    return _replace_once(source,
                         '    mov r1.w, options.x\n    cmp constants.x, lt, lt, r1.w\n'
                         '    jmpc cmp.x, diffuse_color\n',
                         '    jmpu diffuse_enabled, diffuse_color\n', 'glass diffuse selector')


def _skin_mad_variant(source, name):
    # Preserve the first product and all DP3/DP4, weight and degeneracy tests.
    # Apply the position change to color, depth/cloak and Chicago together.
    for register in SKIN_MAD_PROGRAMS[name]:
        result = f'r{register}'
        source = _replace_once(source,
                               f'    mul {result}.xyz, v3.z, {result}\n'
                               f'    mul r4.xyz, v3.w, r4\n    add {result}.xyz, {result}, r4\n',
                               f'    mul {result}.xyz, v3.z, {result}\n'
                               f'    mad {result}.xyz, v3.w, r4, {result}\n',
                               f'{name} weighted blend for {result}')
    return source


def _water_base_variant(source):
    source = _replace_once(source, '.proc main\n',
                           '.alias base_enabled b1\n.proc main\n', 'main entry')
    source = _replace_once(source, '    dp4 position.w, projection[3], r0\n',
                           '    dp4 position.w, projection[3], r0\n'
                           '    jmpu base_enabled, base_only\n', 'water projection tail')
    # The multiply TEV pass reads texture 1 only. Keep all enabled output
    # lanes initialized and preserve position/texture 1 inputs exactly.
    return _replace_once(source, '    mov maps, v2\n    end\n.end\n',
                         '    mov maps, v2\n    end\nbase_only:\n'
                         '    mov color, constants.yyyy\n    mov cube, constants.xxyx\n'
                         '    mov maps, v2\n    end\n.end\n', 'water output tail')


def _instruction_count(data):
    magic, entries = struct.unpack_from('<4sI', data)
    if magic != b'DVLB':
        raise ValueError('Invalid assembled shader header')
    program = (2 + entries) * 4
    magic, _, code_offset, count = struct.unpack_from('<4sIII', data, program)
    if magic != b'DVLP' or program + code_offset + count * 4 > len(data):
        raise ValueError('Invalid assembled shader program')
    return count


def generate(port, out, picasso, *, uv_outputs=False, boolean_selectors=False,
             skin_mad=False, water_base=False, model_uv_pack=False, model_view_pack=False,
             model_quaternion_direct=False, chicago_map_uv=False, model_reflection_direct=False,
             model_homogeneous=False, effect_homogeneous=False, packed_vertices=False):
    if packed_vertices:
        from .packed_skin import packed_skin_source
    shaders = {}
    for source in sorted((port/'shaders').glob('*.v.pica')):
        name = source.name[:-7]
        symbol = 'texture_test_shader' if name == 'texture_test' else 'engine_' + name + '_shader'
        binary = out / (name + '.shbin')
        assembly_source = source
        original = text = source.read_text()
        transforms = []
        if model_quaternion_direct and name in VIEW_PACK_PROGRAMS:
            text = _model_quaternion_variant(text)
            transforms.append('model-quaternion-direct')
        if chicago_map_uv and name in CHICAGO_TRANSFORM_PROGRAMS:
            text = _chicago_map_uv_variant(text)
            transforms.append('chicago-map-uv')
        if uv_outputs and name in UV_OUTPUT_PROGRAMS:
            text = _uv_output_variant(text, UV_OUTPUT_PROGRAMS[name])
            transforms.append('uv-outputs')
        if model_uv_pack and name in BASIC_UV_PROGRAMS:
            text = _basic_model_uv_variant(text)
            transforms.append('model-uv-pack')
        if model_view_pack and name in VIEW_PACK_PROGRAMS:
            text = _model_view_variant(text)
            transforms.append('model-view-pack')
        if model_reflection_direct and name in VIEW_PACK_PROGRAMS:
            text = _model_reflection_variant(text)
            transforms.append('model-reflection-direct')
        elif boolean_selectors and name in BOOLEAN_PROGRAMS:
            text = _boolean_variant(text, name)
            transforms.append('boolean-selectors')
        if skin_mad and name in SKIN_MAD_PROGRAMS:
            text = _skin_mad_variant(text, name)
            transforms.append('skin-mad')
        if model_homogeneous and name in HOMOGENEOUS_MODEL_PROGRAMS:
            text = _model_homogeneous_variant(text, name)
            transforms.append('model-homogeneous')
        if water_base and name == 'water':
            text = _water_base_variant(text)
            transforms.append('water-base')
        if effect_homogeneous and name in HOMOGENEOUS_EFFECT_PROGRAMS:
            text = _effect_homogeneous_variant(text, name)
            transforms.append('effect-homogeneous')
        if packed_vertices:
            packed = packed_skin_source(text, name)
            if packed != text:
                transforms.append('packed-vertices')
                text = packed
        if text != original:
            assembly_source = out / (name + '-' + '-'.join(transforms) + '.v.pica')
            assembly_source.write_text(text)
        subprocess.run([str(picasso), '-o', str(binary), str(assembly_source)], check=True)
        data = binary.read_bytes()
        (out / (symbol + '.h')).write_text(
            'static const unsigned char ' + symbol + '[] __attribute__((aligned(4))) = {'
            + ','.join(hex(x) for x in data) + '};\n')
        shaders[symbol] = {'source': str(source), 'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                           'binary': str(binary), 'sha256': hashlib.sha256(data).hexdigest()}
        if name in HOMOGENEOUS_MODEL_PROGRAMS | HOMOGENEOUS_EFFECT_PROGRAMS:
            shaders[symbol]['instruction_count'] = _instruction_count(data)
        if assembly_source != source:
            shaders[symbol]['transforms'] = transforms
            shaders[symbol]['assembly_source'] = str(assembly_source)
            shaders[symbol]['assembly_source_sha256'] = hashlib.sha256(assembly_source.read_bytes()).hexdigest()
    return shaders


def verify_linked(shaders, elf, old_directories, report):
    image = elf.read_bytes()
    result = {}
    for symbol, entry in shaders.items():
        fresh = Path(entry['binary']).read_bytes()
        result[symbol] = dict(entry, linked=fresh in image)
        for folder in old_directories:
            header = folder / (symbol + '.h')
            if not header.exists():
                continue
            text = header.read_text().split('{', 1)[1].split('}', 1)[0]
            stale = bytes(int(x, 16) for x in re.findall(r'0x[0-9a-fA-F]+', text))
            if stale and stale != fresh:
                assert stale not in image, 'Stale shader linked: ' + str(header)
    assert result['engine_scene_shader']['linked'], 'Correct loading shader absent from executable'
    report.write_text(json.dumps(result, indent=2))
    return result
