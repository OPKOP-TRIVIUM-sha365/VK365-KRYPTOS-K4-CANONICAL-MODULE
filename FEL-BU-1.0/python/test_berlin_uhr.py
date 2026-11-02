#!/usr/bin/env python3
import json
import unittest
from pathlib import Path

from berlin_uhr import BerlinUhrError, bu_encode, bu_validate


def load_kat_document():
    data_path = Path(__file__).resolve().parents[1] / "vectors" / "bu_kat_001.json"
    with data_path.open("r", encoding="utf-8") as fh:
        doc = json.load(fh)
    ids = []
    seen = set()
    for item in doc["vectors"]:
        if "id" not in item:
            raise AssertionError("KAT vector missing id")
        ids.append(item["id"])
        if item["id"] in seen:
            raise AssertionError(f"Duplicate KAT ID: {item['id']}")
        seen.add(item["id"])
        for key in ("timestamp", "berlin", "rgb", "hex"):
            if key not in item:
                raise AssertionError(f"KAT vector {item['id']} missing key: {key}")
    if len(doc["vectors"]) != 5:
        raise AssertionError("Expected 5 canonical KAT vectors")
    if len(ids) != len(set(ids)):
        raise AssertionError("Duplicate KAT IDs detected")
    return doc


class TestKATCorpus(unittest.TestCase):
    def test_unique_ids(self):
        doc = load_kat_document()
        ids = [item["id"] for item in doc["vectors"]]
        self.assertEqual(len(ids), len(set(ids)))

    def test_canonical_kat_vectors(self):
        doc = load_kat_document()
        for item in doc["vectors"]:
            with self.subTest(item=item["id"]):
                self.assertEqual(bu_encode(item["timestamp"]), item["berlin"])
                self.assertTrue(bu_validate(item["timestamp"], item["berlin"]))

    def test_duplicate_kat_ids_are_rejected(self):
        dup = {
            "vectors": [
                {"id": "DUP-1", "timestamp": "2026-11-01T00:00:00Z", "berlin": "000000000000000000000000", "rgb": [0, 0, 0], "hex": "000000"},
                {"id": "DUP-1", "timestamp": "1990-11-03T05:58:00Z", "berlin": "010000000111111111111110", "rgb": [63, 255, 238], "hex": "3FFFEE"},
            ]
        }
        with self.assertRaises(AssertionError):
            ids = []
            seen = set()
            for item in dup["vectors"]:
                if item["id"] in seen:
                    raise AssertionError("Duplicate KAT ID detected")
                seen.add(item["id"])
                ids.append(item["id"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
