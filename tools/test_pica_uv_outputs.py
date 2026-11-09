"""Check assembled UV lane equivalence: PICASSO=/path/to/picasso python3 -m unittest discover -s tools -p test_pica_uv_outputs.py.

This proves the local instruction/layout change, not PICA arithmetic, image
parity, or a scene performance improvement. Temporary build products are removed.
"""
import importlib.util
import os
from pathlib import Path
import shutil
import struct
import tempfile
import unittest


PORT = Path(__file__).resolve().parents[1] / 'port/n3ds'
SPEC = importlib.util.spec_from_file_location('pica_shaders', PORT / 'tools/shaders.py')
SHADERS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(SHADERS)


def program(data):
    magic, entries, dvle = struct.unpack_from('<4sII', data)
    if magic != b'DVLB' or entries != 1:
        raise ValueError('Expected a single vertex program')
    dvlp = (2 + entries) * 4
    magic, _, code, count, descriptors, desc_count = struct.unpack_from('<4s5I', data, dvlp)
    if magic != b'DVLP':
        raise ValueError('Missing program')
    instructions = struct.unpack_from(f'<{count}I', data, dvlp + code)
    operands = [struct.unpack_from('<I', data, dvlp + descriptors + 8 * i)[0] for i in range(desc_count)]
    # Only the entry-end address is expected to move in DVLE metadata.
    return instructions, operands, data[dvle:dvle + 12] + data[dvle + 16:]


def lanes(word, descriptor):
    mask = descriptor & 15
    src1, src2 = (descriptor >> 4) & 511, (descriptor >> 13) & 511
    return {i: (word & ~127, src1 & 1, (src1 >> (7 - 2 * i)) & 3,
                src2 & 1, (src2 >> (7 - 2 * i)) & 3)
            for i in range(4) if mask & (8 >> i)}


def output_semantics(data):
    """Mirror the exported lane order in libctru's DVLE_GenerateOutmap."""
    dvle, = struct.unpack_from('<I', data, 8)
    offset, count = struct.unpack_from('<II', data, dvle + 40)
    widths = {0: 4, 1: 4, 2: 4, 3: 2, 4: 1, 5: 2, 6: 2, 8: 3}
    result = {}
    for kind, register, mask in struct.iter_unpack('<HHB3x', data[dvle+offset:dvle+offset+count*8]):
        selected = [lane for lane in range(4) if mask & (1 << lane)]
        for channel, lane in enumerate(selected[:widths[kind]]):
            result[register, lane] = kind, channel
    return result


def semantic_writes(instructions, descriptors, semantics):
    result = {}
    for word in instructions:
        register = (word >> 21) & 31
        for lane, (opcode_inputs, *operands) in lanes(word, descriptors[word & 127]).items():
            if (register, lane) in semantics:
                semantic = semantics[register, lane]
                if semantic in result:
                    raise ValueError('Output semantic written twice')
                if word >> 26 == 19:  # MOV has no second source.
                    operands = operands[:2]
                result[semantic] = (opcode_inputs & ~(31 << 21), *operands)
    return result


class UVOutputs(unittest.TestCase):
    def test_rejects_changed_or_duplicate_sequences(self):
        source = (PORT / 'shaders/skin_lit.v.pica').read_text()
        with self.assertRaises(ValueError):
            SHADERS._uv_output_variant(source.replace('options.xyxy', 'options.zwzw'), 'texcoord')
        with self.assertRaises(ValueError):
            SHADERS._uv_output_variant(source + source, 'texcoord')

    def test_basic_uv_pack_exports_identical_texcoords(self):
        picasso = os.environ.get('PICASSO') or shutil.which('picasso')
        if not picasso:
            self.skipTest('Set PICASSO to assemble both variants')
        with tempfile.TemporaryDirectory(prefix='halo-pica-basic-uv-') as directory:
            reference = Path(directory) / 'reference'
            candidate = Path(directory) / 'candidate'
            reference.mkdir()
            candidate.mkdir()
            SHADERS.generate(PORT, reference, picasso)
            SHADERS.generate(PORT, candidate, picasso, model_uv_pack=True)
            changed = set()
            for path in reference.glob('*.shbin'):
                before, after = path.read_bytes(), (candidate / path.name).read_bytes()
                if before == after:
                    continue
                changed.add(path.stem)
                with self.subTest(program=path.stem):
                    old, old_desc, _ = program(before)
                    new, new_desc, _ = program(after)
                    self.assertEqual(len(old), len(new) + 3)
                    self.assertEqual(old[:-6], new[:-3])
                    self.assertEqual(old[-1], new[-1])
                    for op in old[:-6]:
                        if op >> 26 < 32 or op >> 26 in (46, 47):
                            relevant = 0x1fff if op >> 26 in (18, 19) else 0xffffffff
                            self.assertEqual(old_desc[op & 127] & relevant, new_desc[op & 127] & relevant)
                    old_map, new_map = map(output_semantics, (before, after))
                    self.assertEqual(sorted(old_map.values()), sorted(new_map.values()))
                    self.assertEqual({k: v for k, v in old_map.items() if v[0] < 3},
                                     {k: v for k, v in new_map.items() if v[0] < 3})
                    self.assertEqual(len({k[0] for k in old_map}), len({k[0] for k in new_map}) + 1)
                    before_uv = semantic_writes(old[-6:-1], old_desc, old_map)
                    after_uv = semantic_writes(new[-3:-1], new_desc, new_map)
                    self.assertEqual(set(before_uv), {(kind, lane) for kind in (3, 5, 6) for lane in range(2)})
                    self.assertEqual(before_uv, after_uv)
            self.assertEqual(changed, {'rigid', 'skin'})

    def test_model_view_pack_preserves_exported_xyz(self):
        picasso = os.environ.get('PICASSO') or shutil.which('picasso')
        if not picasso:
            self.skipTest('Set PICASSO to assemble both variants')
        with tempfile.TemporaryDirectory(prefix='halo-pica-view-') as directory:
            reference = Path(directory) / 'reference'
            candidate = Path(directory) / 'candidate'
            reference.mkdir()
            candidate.mkdir()
            SHADERS.generate(PORT, reference, picasso)
            SHADERS.generate(PORT, candidate, picasso, model_view_pack=True)
            changed = set()
            for path in reference.glob('*.shbin'):
                before, after = path.read_bytes(), (candidate / path.name).read_bytes()
                if before == after:
                    continue
                changed.add(path.stem)
                with self.subTest(program=path.stem):
                    old, old_desc, old_meta = program(before)
                    new, new_desc, new_meta = program(after)
                    self.assertEqual(old_meta, new_meta)
                    self.assertEqual(len(old), len(new) + 1)
                    outmap = output_semantics(before)
                    self.assertEqual(outmap, output_semantics(after))
                    view = {slot: semantic for slot, semantic in outmap.items() if semantic[0] == 8}
                    self.assertEqual(set(view.values()), {(8, 0), (8, 1), (8, 2)})
                    view_register, _ = next(iter(view))
                    first, last = [i for i, op in enumerate(old)
                                   if (op >> 26, (op >> 21) & 31) == (19, view_register)]
                    self.assertEqual(last, first+1)
                    self.assertEqual(semantic_writes(old[first:last+1], old_desc, view),
                                     semantic_writes(new[first:first+1], new_desc, view))
                    old_eye = lanes(old[first-1], old_desc[old[first-1] & 127])
                    new_eye = lanes(new[first-1], new_desc[new[first-1] & 127])
                    self.assertEqual(set(old_eye), {0, 1, 2})
                    self.assertEqual(set(new_eye), {0, 1, 2, 3})
                    self.assertEqual(old_eye, {k: new_eye[k] for k in old_eye})
            self.assertEqual(changed, {'rigid_reflect', 'skin_reflect'})

    def test_assembled_lane_routing_and_unaffected_programs(self):
        picasso = os.environ.get('PICASSO') or shutil.which('picasso')
        if not picasso:
            self.skipTest('Set PICASSO to assemble both variants')
        with tempfile.TemporaryDirectory(prefix='halo-pica-uv-') as directory:
            root = Path(directory)
            reference, candidate = root / 'reference', root / 'candidate'
            reference.mkdir()
            candidate.mkdir()
            SHADERS.generate(PORT, reference, picasso)
            SHADERS.generate(PORT, candidate, picasso, uv_outputs=True)
            changed = set()
            for path in reference.glob('*.shbin'):
                before, after = path.read_bytes(), (candidate / path.name).read_bytes()
                if before == after:
                    continue
                changed.add(path.stem)
                with self.subTest(program=path.stem):
                    old, old_desc, old_meta = program(before)
                    new, new_desc, new_meta = program(after)
                    self.assertEqual(old_meta, new_meta)
                    self.assertEqual(len(old), len(new) + 1)
                    self.assertEqual(old[:-3], new[:-2])
                    self.assertEqual(old[-1], new[-1])
                    for op in old[:-3]:
                        if op >> 26 < 32 or op >> 26 in (46, 47):
                            index = op & 127
                            relevant = 0x1fff if op >> 26 in (18, 19) else 0xffffffff
                            self.assertEqual(old_desc[index] & relevant, new_desc[index] & relevant)
                    expected = {}
                    for op in old[-3:-1]:
                        self.assertEqual(op >> 26, 8)  # MUL
                        output = lanes(op, old_desc[op & 127])
                        self.assertFalse(expected.keys() & output.keys())
                        expected.update(output)
                    self.assertEqual(len(expected), 4)
                    self.assertEqual(expected, lanes(new[-2], new_desc[new[-2] & 127]))
            self.assertEqual(changed, {'rigid_lit', 'rigid_reflect', 'skin_lit', 'skin_reflect'})


if __name__ == '__main__':
    unittest.main()
