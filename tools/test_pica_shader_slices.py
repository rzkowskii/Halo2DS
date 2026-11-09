"""Check separately assembled PICA selector, skin blend, and water-base variants."""

import os
from pathlib import Path
import re
import shutil
import sys
import tempfile
import unittest

from test_pica_uv_outputs import PORT, output_semantics, program


def assembled(directory, name):
    return program((directory / f'{name}.shbin').read_bytes())


def jmpu(word):
    return ((word >> 22) & 15, bool(word & 1), (word >> 10) & 4095)


def ordinary(word, descriptors):
    return word & ~127, descriptors[word & 127] & ((1 << 22) - 1)


class ShaderSlices(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        picasso = os.environ.get('PICASSO') or shutil.which('picasso')
        if not picasso:
            raise unittest.SkipTest('Set PICASSO to assemble the shader variants')
        cls.temp = tempfile.TemporaryDirectory(prefix='halo-pica-slices-')
        cls.addClassCleanup(cls.temp.cleanup)
        root = Path(cls.temp.name)
        cls.variants = {}
        for name, options in {
            'reference': {}, 'boolean': {'boolean_selectors': True}, 'mad': {'skin_mad': True},
            'water': {'water_base': True}, 'quaternion': {'model_quaternion_direct': True}, 'combined': {
                'uv_outputs': True, 'boolean_selectors': True, 'skin_mad': True,
                'water_base': True, 'model_uv_pack': True, 'model_view_pack': True,
                'model_quaternion_direct': True, 'packed_vertices': True,
            },
            'chicago': {'chicago_map_uv': True},
            'reflection': {'model_reflection_direct': True},
            'reflection-combined': {'model_reflection_direct': True, 'boolean_selectors': True,
                                    'model_view_pack': True, 'model_quaternion_direct': True,
                                    'uv_outputs': True, 'packed_vertices': True},
            'homogeneous': {'model_homogeneous': True},
            'homogeneous-packed': {'model_homogeneous': True, 'model_view_pack': True,
                                   'skin_mad': True, 'packed_vertices': True,
                                   'model_reflection_direct': True, 'chicago_map_uv': True},
            'effect-homogeneous': {'effect_homogeneous': True},
            'effect-combined': {'effect_homogeneous': True, 'model_homogeneous': True,
                                'boolean_selectors': True, 'water_base': True},
        }.items():
            directory = root / name
            directory.mkdir()
            cls.variants[name] = directory
            sys.path.insert(0, str(PORT))
            try:
                from tools import shaders
                shaders.generate(PORT, directory, picasso, **options)
            finally:
                sys.path.pop(0)

    def changed(self, variant):
        return {p.stem for p in self.variants['reference'].glob('*.shbin')
                if p.read_bytes() != (self.variants[variant] / p.name).read_bytes()}

    def test_model_homogeneous_keeps_position_rows_and_palette_addressing(self):
        models = {base + suffix for base in ('rigid', 'skin')
                  for suffix in ('', '_lit', '_reflect', '_chicago', '_depth')}
        self.assertEqual(self.changed('homogeneous'), models)
        for name in models:
            with self.subTest(program=name):
                old, old_desc, old_meta = assembled(self.variants['reference'], name)
                new, new_desc, new_meta = assembled(self.variants['homogeneous'], name)
                packed, _, _ = assembled(self.variants['homogeneous-packed'], name)
                self.assertEqual(old_meta, new_meta)
                self.assertEqual(len(old) - len(new), 3)
                # DPHI puts the input first and the uniform second. The
                # address-register selector and destination mask stay put.
                old_dots = [(op, old_desc[op & 127] & 15) for op in old
                            if op >> 26 == 2 and (op >> 21) & 31 in (0, 17, 20)]
                new_dots = [(op, new_desc[op & 127] & 15) for op in new if op >> 26 == 24]
                self.assertEqual(len(old_dots), 10 if name.startswith('skin') else 7)
                self.assertEqual(len(old_dots), len(new_dots))
                self.assertEqual(sum(op >> 26 == 24 for op in packed), len(new_dots))
                for (before, mask), (after, new_mask) in zip(old_dots, new_dots):
                    self.assertEqual((before >> 19) & 127, (after >> 19) & 127)
                    self.assertEqual((before >> 12) & 127, (after >> 7) & 127)
                    self.assertEqual((after >> 14) & 31, 17 if ((before >> 21) & 31) == 0 else 0)
                    self.assertEqual(mask, new_mask)

    def test_effect_homogeneous_preserves_projection_pose_and_screen_rows(self):
        expected = {'effect_packed': 4, 'chicago': 4, 'hologram': 7, 'glass': 7, 'water': 4}
        self.assertEqual(self.changed('effect-homogeneous'), set(expected))
        for name, dots in expected.items():
            with self.subTest(program=name):
                old, old_desc, old_meta = assembled(self.variants['reference'], name)
                new, new_desc, new_meta = assembled(self.variants['effect-homogeneous'], name)
                combined, _, _ = assembled(self.variants['effect-combined'], name)
                self.assertEqual(old_meta, new_meta)
                self.assertEqual(len(old) - len(new), 1 if name == 'glass' else 2)
                old_dots = [op for op in old if op >> 26 == 2]
                new_dots = [op for op in new if op >> 26 == 24]
                self.assertEqual((len(old_dots), len(new_dots)), (dots, dots))
                self.assertEqual(sum(op >> 26 == 24 for op in combined), dots)
                for before, after in zip(old_dots, new_dots):
                    self.assertEqual((before >> 19) & 127, (after >> 19) & 127)
                    self.assertEqual((before >> 12) & 127, (after >> 7) & 127)
                    self.assertEqual((after >> 14) & 31, 16 if name == 'glass' else 0)
                    self.assertEqual(old_desc[before & 127] & 15, new_desc[after & 127] & 15)

    def test_boolean_slots_polarities_and_targets(self):
        expected = {
            'rigid_reflect': (2, ((0, True, 'diffuse_only'),)),
            'skin_reflect': (2, ((0, True, 'diffuse_only'),)),
            'water': (2, ((0, True, 'wave_ready'),)),
            'glass': (6, ((0, True, 'pose_ready'), (1, False, 'diffuse_color'))),
        }
        self.assertEqual(self.changed('boolean'), set(expected))
        for name, (saving, selectors) in expected.items():
            with self.subTest(program=name):
                before, _, _ = assembled(self.variants['reference'], name)
                after, _, _ = assembled(self.variants['boolean'], name)
                self.assertEqual(len(before) - len(after), saving)
                source = next(self.variants['boolean'].glob(f'{name}-*.v.pica')).read_text()
                addresses, instruction = {}, 0
                for line in source.splitlines():
                    line = line.partition(';')[0].strip()
                    if line.endswith(':'):
                        addresses[line[:-1]] = instruction
                    elif line and not line.startswith('.'):
                        instruction += 1
                self.assertEqual(instruction, len(after))
                self.assertEqual([jmpu(op) for op in after if op >> 26 == 45],
                                 [(slot, inverted, addresses[label]) for slot, inverted, label in selectors])

    def test_renderer_bool_locations_match_citro3d_contract(self):
        expected = {'MODEL_REFLECTION_BOOL': 0, 'WATER_WAVES_BOOL': 0, 'WATER_BASE_BOOL': 1,
                    'GLASS_RIGID_BOOL': 0, 'GLASS_DIFFUSE_BOOL': 1}
        # Pinned Citro3D accepts uniform locations (0x68 + bit), while PICA's
        # JMPU operand is the raw Boolean bit checked above.
        base = 0x68
        citro3d = Path(os.environ.get('CITRO3D', PORT.parents[1] / 'build/dependencies/citro3d-9f21cf7'))
        uniforms = citro3d / 'include/c3d/uniforms.h'
        if uniforms.is_file():
            setter = uniforms.read_text().split('void C3D_BoolUnifSet(', 1)[1]
            self.assertEqual(int(re.search(r'id\s*-=\s*(0x[0-9a-f]+);', setter)[1], 16), base)
        renderer = (PORT / 'src/engine_gpu_renderer.c').read_text()
        for name, slot in expected.items():
            location = re.search(rf'\b{name}\s*=\s*(0x[0-9a-f]+|\d+)', renderer)
            self.assertIsNotNone(location, name)
            self.assertEqual(int(location[1], 0), base + slot, name)

    def test_mad_preserves_weight_swizzles_and_first_product(self):
        expected = {'skin': 2, 'skin_lit': 2, 'skin_reflect': 2, 'skin_depth': 1, 'skin_chicago': 1}
        self.assertEqual(self.changed('mad'), set(expected))
        for name, count in expected.items():
            with self.subTest(program=name):
                old, old_desc, old_meta = assembled(self.variants['reference'], name)
                new, new_desc, new_meta = assembled(self.variants['mad'], name)
                self.assertEqual(new_meta, old_meta)
                self.assertEqual(len(old) - len(new), count)
                old_multiply = [
                    i for i, op in enumerate(old) if
                    (op >> 26, (op >> 21) & 31, (op >> 12) & 127, (op >> 7) & 31) == (8, 20, 3, 20)
                ]
                mad = [i for i, op in enumerate(new) if op >> 26 in range(56, 64)]
                self.assertEqual(len(old_multiply), count)
                self.assertEqual(len(mad), count)
                for i, j in zip(old_multiply, mad):
                    # The assembler encodes MAD's third source and descriptor
                    # in fields distinct from ordinary arithmetic.
                    word = new[j]
                    result = (old[i + 1] >> 21) & 31
                    self.assertEqual((word >> 24) & 31, result)
                    self.assertEqual(((word >> 17) & 31, (word >> 10) & 127, (word >> 5) & 31),
                                     (3, 20, result))
                    self.assertEqual(ordinary(old[i - 1], old_desc), ordinary(new[j - 1], new_desc))
                    old_operand = old_desc[old[i] & 127]
                    new_operand = new_desc[word & 31]
                    self.assertEqual(old_operand & ((1 << 22) - 1), new_operand & ((1 << 22) - 1))
                    # Third source is the unmodified first product, with
                    # the same identity swizzle as the reference ADD.
                    self.assertEqual((new_operand >> 22) & 511,
                                     (old_desc[old[i + 1] & 127] >> 4) & 511)

    def test_water_base_branches_after_projection_and_keeps_base_uv(self):
        self.assertEqual(self.changed('water'), {'water'})
        old, old_desc, old_meta = assembled(self.variants['reference'], 'water')
        new, new_desc, new_meta = assembled(self.variants['water'], 'water')
        self.assertEqual(old_meta, new_meta)
        self.assertEqual([ordinary(op, old_desc) for op in old[:6]],
                         [ordinary(op, new_desc) for op in new[:6]])
        self.assertEqual(new[6] >> 26, 45)
        self.assertEqual(jmpu(new[6]), (1, False, len(new) - 4))
        self.assertEqual(len(new), len(old) + 5)
        self.assertEqual(ordinary(old[-2], old_desc), ordinary(new[-2], new_desc))
        self.assertEqual(new[-1], old[-1])
        combined, _, _ = assembled(self.variants['combined'], 'water')
        self.assertEqual([jmpu(op)[:2] for op in combined if op >> 26 == 45],
                         [(1, False), (0, True)])

    def test_model_quaternion_writes_original_values_at_either_exit(self):
        models = {'rigid_reflect', 'skin_reflect'}
        self.assertEqual(self.changed('quaternion'), models)
        destination_mask = 31 << 21
        branch_target_mask = 4095 << 10
        for name in models:
            with self.subTest(program=name):
                old, old_desc, old_meta = assembled(self.variants['reference'], name)
                new, new_desc, new_meta = assembled(self.variants['quaternion'], name)
                self.assertEqual(old_meta, new_meta)
                self.assertEqual(len(old), len(new))
                self.assertEqual(old[:-13], new[:-13])
                self.assertEqual([ordinary(op, old_desc) for op in old[-3:-1]],
                                 [ordinary(op, new_desc) for op in new[-13:-11]])
                self.assertEqual([ordinary(op, old_desc) for op in old[-13:-9]],
                                 [ordinary(op, new_desc) for op in new[-11:-7]])
                self.assertEqual(ordinary(old[-8], old_desc), ordinary(new[-7], new_desc))
                self.assertEqual(old[-7] & ~branch_target_mask, new[-6] & ~branch_target_mask)
                self.assertEqual((old[-7] & branch_target_mask) >> 10, len(old) - 4)
                self.assertEqual((new[-6] & branch_target_mask) >> 10, len(new) - 2)
                self.assertEqual(ordinary(old[-6], old_desc), ordinary(new[-5], new_desc))
                output = old[-4] & destination_mask
                for reference, candidate, operand_mask in ((old[-5], new[-4], (1 << 22) - 1),
                                                            (old[-9], new[-2], (1 << 13) - 1)):
                    self.assertEqual(candidate & destination_mask, output)
                    self.assertEqual(reference & ~(destination_mask | 127),
                                     candidate & ~(destination_mask | 127))
                    self.assertEqual(old_desc[reference & 127] & operand_mask,
                                     new_desc[candidate & 127] & operand_mask)
                self.assertEqual(new[-3], old[-1])
                self.assertEqual(new[-1], old[-1])

    def test_chicago_branches_skip_only_unused_uv_rows(self):
        names = {'rigid_chicago', 'skin_chicago'}
        self.assertEqual(self.changed('chicago'), names)
        for name in names:
            with self.subTest(program=name):
                before = (self.variants['reference'] / f'{name}.shbin').read_bytes()
                after = (self.variants['chicago'] / f'{name}.shbin').read_bytes()
                old, old_desc, old_meta = program(before)
                new, new_desc, new_meta = program(after)
                self.assertEqual(new_meta, old_meta)
                outmap = output_semantics(before)
                self.assertEqual(outmap, output_semantics(after))
                self.assertEqual({semantic for semantic in outmap.values() if semantic[0] >= 3},
                                 {(kind, lane) for kind in (3, 5, 6) for lane in range(2)})
                self.assertEqual(old[:-10], new[:-12])
                for op in old[:-10]:
                    if op >> 26 < 32:
                        self.assertEqual(ordinary(op, old_desc), ordinary(op, new_desc))
                for stock, candidate in zip((-10, -9, -7, -6, -4, -3),
                                            (-12, -11, -9, -8, -6, -5)):
                    self.assertEqual(ordinary(old[stock], old_desc), ordinary(new[candidate], new_desc))
                self.assertEqual(jmpu(new[-10]), (0, True, len(new) - 3))
                self.assertEqual(jmpu(new[-7]), (1, True, len(new) - 2))
                self.assertEqual(new[-4], old[-1])
                self.assertEqual(new[-1], old[-1])
                for cleared, used in ((new[-3], new[-9]), (new[-2], new[-6])):
                    self.assertEqual(cleared >> 26, 19)  # MOV
                    self.assertEqual((cleared >> 21) & 31, (used >> 21) & 31)
                    self.assertEqual(new_desc[cleared & 127] & 15, 0b1100)

    def test_model_reflection_writes_outputs_in_each_branch(self):
        names = {'rigid_reflect', 'skin_reflect'}
        self.assertEqual(self.changed('reflection'), names)
        for name in names:
            with self.subTest(program=name):
                paths = [self.variants[mode] / f'{name}.shbin' for mode in ('reference', 'reflection')]
                before, after = (path.read_bytes() for path in paths)
                old, _, old_meta = program(before)
                new, desc, new_meta = program(after)
                self.assertEqual(old_meta, new_meta)
                self.assertEqual(output_semantics(before), output_semantics(after))
                self.assertEqual(len(old)-len(new), 4)
                index, = (i for i, word in enumerate(new) if word >> 26 == 39)  # IFU
                branch = new[index]
                other = (branch >> 10) & 4095
                self.assertEqual(((branch >> 22) & 15, other-index-1, branch & 255), (0, 10, 3))
                outmap = output_semantics(after)
                color, = {reg for (reg, _), (kind, _) in outmap.items() if kind == 2}
                cube, = {reg for (reg, _), (kind, _) in outmap.items() if kind == 4}
                for offset, register, mask in ((index+8, cube, 0b1110), (index+10, color, 0b1111),
                                               (other, color, 0b1111), (other+1, cube, 0b1100),
                                               (other+2, cube, 0b0010)):
                    word = new[offset]
                    self.assertEqual((word >> 21) & 31, register)
                    self.assertEqual(desc[word & 127] & 15, mask)
                combined, _, _ = assembled(self.variants['reflection-combined'], name)
                self.assertEqual(sum(word >> 26 == 39 for word in combined), 1)
                self.assertFalse(any(word >> 26 == 45 for word in combined))


if __name__ == '__main__':
    unittest.main()
