"""Check generated presentation records without proprietary client fixtures."""
import importlib.util
import struct
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('display', Path(__file__).resolve().parents[1] / 'tools/build_display_dbc.py')
display = importlib.util.module_from_spec(spec)
spec.loader.exec_module(display)


class DisplayTest(unittest.TestCase):
    def test_preserves_existing_and_uses_only_dummy_auras(self):
        row = [0] * 234
        row[0] = 42
        row[136] = 1
        original = struct.pack('<4s4I', b'WDBC', 1, 234, 936, 7) + struct.pack('<234I', *row) + b'\0Other\0'
        output = display.transform(original)
        rows, strings = display.decode(output)
        self.assertEqual(rows[0], row)
        self.assertEqual(strings[:7], b'\0Other\0')
        self.assertEqual({r[0] for r in rows[1:]}, {910100, 910101})
        for r in rows[1:]:
            self.assertEqual(r[71:74], [6, 0, 0])
            self.assertEqual(r[95:98], [4, 0, 0])
            self.assertEqual(r[116:119], [0, 0, 0])
            self.assertTrue(r[4] & 0x80000000)
            self.assertTrue(r[7] & 0x00100000)
            self.assertGreater(r[187], 0)
        with self.assertRaises(ValueError):
            display.transform(output)

    def test_rejects_invalid_inputs(self):
        with self.assertRaises(ValueError):
            display.definitions(rest_id=42, reward_id=42)
        bad = struct.pack('<4s4I', b'WDBC', 0, 235, 940, 1) + b'\0'
        with self.assertRaises(ValueError):
            display.decode(bad)


if __name__ == '__main__':
    unittest.main()
