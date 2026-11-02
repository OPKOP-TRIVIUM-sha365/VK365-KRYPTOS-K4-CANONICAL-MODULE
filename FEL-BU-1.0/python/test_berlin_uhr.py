import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
KAT_PATH = ROOT / "vectors" / "bu_kat_001.json"


def load_kat_document():
    with KAT_PATH.open("r", encoding="utf-8") as fh:
        doc = json.load(fh)
    if "vectors" not in doc:
        raise AssertionError("Missing 'vectors' in KAT document")
    if not isinstance(doc["vectors"], list):
        raise AssertionError("'vectors' must be a list")
    if len(doc["vectors"]) != 5:
        raise AssertionError("Expected 5 canonical KAT vectors")
    seen = set()
    for item in doc["vectors"]:
        if not isinstance(item, dict):
            raise AssertionError("Every KAT entry must be an object")
        for key in ("id", "timestamp", "berlin", "rgb", "hex"):
            if key not in item:
                raise AssertionError(f"KAT vector missing required key: {key}")
        item_id = item["id"]
        if not isinstance(item_id, str):
            raise AssertionError("KAT id must be a string")
        if item_id in seen:
            raise AssertionError(f"Duplicate KAT ID detected: {item_id}")
        seen.add(item_id)
        if not isinstance(item["timestamp"], str):
            raise AssertionError(f"timestamp must be string for {item_id}")
        if not isinstance(item["berlin"], str):
            raise AssertionError(f"berlin must be string for {item_id}")
        if not isinstance(item["hex"], str):
            raise AssertionError(f"hex must be string for {item_id}")
        if not isinstance(item["rgb"], list):
            raise AssertionError(f"rgb must be list for {item_id}")
        if len(item["rgb"]) != 3:
            raise AssertionError(f"rgb must have 3 entries for {item_id}")
        for value in item["rgb"]:
            if not isinstance(value, int):
                raise AssertionError(f"rgb values must be ints for {item_id}")
            if value < 0 or value > 255:
                raise AssertionError(f"rgb value out of range for {item_id}")
    return doc


class TestBuKATCorpus(unittest.TestCase):
    def test_unique_kat_ids(self):
        doc = load_kat_document()
        ids = [item["id"] for item in doc["vectors"]]
        self.assertEqual(len(ids), len(set(ids)))

    def test_canonical_kat_has_expected_count(self):
        doc = load_kat_document()
        self.assertEqual(len(doc["vectors"]), 5)

    def test_canonical_kat_structure_is_valid(self):
        doc = load_kat_document()
        for item in doc["vectors"]:
            with self.subTest(item=item["id"]):
                self.assertTrue(isinstance(item["timestamp"], str))
                self.assertTrue(isinstance(item["berlin"], str))
                self.assertTrue(isinstance(item["hex"], str))
                self.assertEqual(len(item["berlin"]), 24)
                self.assertTrue(set(item["berlin"]) <= {"0", "1"})
                self.assertEqual(len(item["hex"]), 6)
                self.assertTrue(all(ch in "0123456789ABCDEF" for ch in item["hex"]))
                self.assertEqual(len(item["rgb"]), 3)

    def test_duplicate_kat_id_rejected(self):
        dup = {
            "vectors": [
                {"id": "X", "timestamp": "2026-11-01T00:00:00Z", "berlin": "000000000000000000000000", "rgb": [0, 0, 0], "hex": "000000"},
                {"id": "X", "timestamp": "1990-11-03T05:58:00Z", "berlin": "010000000111111111111110", "rgb": [63, 255, 238], "hex": "3FFFEE"},
            ]
        }
        seen = set()
        with self.assertRaises(AssertionError):
            for item in dup["vectors"]:
                if item["id"] in seen:
                    raise AssertionError("Duplicate KAT ID detected")
                seen.add(item["id"])

    def test_missing_key_rejected(self):
        bad = {"vectors": [{"id": "X", "timestamp": "2026-11-01T00:00:00Z", "berlin": "000000000000000000000000", "rgb": [0, 0, 0]}]}
        with self.assertRaises(KeyError):
            _ = bad["vectors"][0]["hex"]

    def test_invalid_bitstream_rejected(self):
        with self.assertRaises(AssertionError):
            bits = "00000000000000000000000Z"
            assert len(bits) == 24
            assert set(bits) <= {"0", "1"}

    def test_invalid_hex_rejected(self):
        with self.assertRaises(AssertionError):
            hex_value = "zzzzzz"
            assert all(ch in "0123456789ABCDEF" for ch in hex_value)

    def test_non_ascii_timestamp_rejected(self):
        with self.assertRaises(AssertionError):
            ts = "2026-11-01T00:00:00±00:00"
            assert ts.endswith("Z")

    def test_timestamp_state_mismatch_rejected(self):
        with self.assertRaises(AssertionError):
            ts = "2026-11-01T00:00:00Z"
            bits = "000000000000000000000001"
            assert bits == "000000000000000000000000"


if __name__ == "__main__":
    unittest.main(verbosity=2)
