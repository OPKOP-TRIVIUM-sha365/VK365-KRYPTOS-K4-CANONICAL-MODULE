#!/usr/bin/env python3
import json
import unittest
from pathlib import Path

from berlin_uhr import (
    BerlinUhrError,
    bu_encode,
    bu_validate,
    cmap_hex_from_bits,
    cmap_rgb_from_bits,
    validate_berlin_state,
)

vec_path = Path(__file__).resolve().parents[1] / "vectors" / "bu_kat_001.json"
with vec_path.open("r", encoding="utf-8") as fh:
    KAT = [
        (item["timestamp"], item["berlin"]) for item in json.load(fh)["vectors"]
    ]

EXPECTED_RGB_HEX = {}
with vec_path.open("r", encoding="utf-8") as fh:
    for item in json.load(fh)["vectors"]:
        EXPECTED_RGB_HEX[item["timestamp"]] = (tuple(item["rgb"]), item["hex"])


class TestBerlinUhr(unittest.TestCase):
    def test_kat_vectors(self):
        for timestamp, expected in KAT:
            with self.subTest(timestamp=timestamp):
                self.assertEqual(bu_encode(timestamp), expected)
                self.assertTrue(bu_validate(timestamp, expected))

    def test_cmap_kat_vectors(self):
        for timestamp, bits in KAT:
            with self.subTest(timestamp=timestamp):
                expected_rgb, expected_hex = EXPECTED_RGB_HEX[timestamp]
                self.assertEqual(cmap_rgb_from_bits(bits), expected_rgb)
                self.assertEqual(cmap_hex_from_bits(bits), expected_hex)

    def test_determinism(self):
        for timestamp, _ in KAT:
            self.assertEqual(bu_encode(timestamp), bu_encode(timestamp))

    def test_invalid_calendar_date(self):
        invalid = [
            "2026-02-29T12:00:00Z",
            "2026-04-31T12:00:00Z",
            "2026-13-01T12:00:00Z",
            "2026-00-01T12:00:00Z",
            "0000-01-01T12:00:00Z",
        ]
        for timestamp in invalid:
            with self.subTest(timestamp=timestamp):
                with self.assertRaises(BerlinUhrError):
                    bu_encode(timestamp)

    def test_year_0001_and_9999_valid(self):
        self.assertEqual(len(bu_encode("0001-01-01T00:00:00Z")), 24)
        self.assertEqual(len(bu_encode("9999-12-31T23:59:59Z")), 24)

    def test_invalid_time(self):
        invalid = [
            "2026-11-01T24:00:00Z",
            "2026-11-01T12:60:00Z",
            "2026-11-01T12:00:60Z",
        ]
        for timestamp in invalid:
            with self.subTest(timestamp=timestamp):
                with self.assertRaises(BerlinUhrError):
                    bu_encode(timestamp)

    def test_fractional_seconds_rejected(self):
        with self.assertRaises(BerlinUhrError):
            bu_encode("2026-11-01T12:00:00.1Z")

    def test_timezone_offset_rejected(self):
        with self.assertRaises(BerlinUhrError):
            bu_encode("2026-11-01T12:00:00+01:00")

    def test_timestamp_without_z_rejected(self):
        with self.assertRaises(BerlinUhrError):
            bu_encode("2026-11-01T12:00:00")

    def test_non_ascii_rejected(self):
        with self.assertRaises(BerlinUhrError):
            bu_encode("２０２６-11-01T12:00:00Z")

    def test_berlin_wrong_length(self):
        with self.assertRaises(BerlinUhrError):
            validate_berlin_state("0" * 23)

    def test_berlin_invalid_character(self):
        with self.assertRaises(BerlinUhrError):
            validate_berlin_state("0" * 23 + "X")

    def test_non_prefix_five_hour(self):
        bits = "0" + "1010" + "0000" + "0" * 11 + "0000"
        with self.assertRaises(BerlinUhrError):
            validate_berlin_state(bits)

    def test_non_prefix_one_hour(self):
        bits = "0" + "0000" + "1010" + "0" * 11 + "0000"
        with self.assertRaises(BerlinUhrError):
            validate_berlin_state(bits)

    def test_non_prefix_five_minute(self):
        bits = "0" + "0000" + "0000" + "10100000000" + "0000"
        with self.assertRaises(BerlinUhrError):
            validate_berlin_state(bits)

    def test_non_prefix_one_minute(self):
        bits = "0" + "0000" + "0000" + "0" * 11 + "1010"
        with self.assertRaises(BerlinUhrError):
            validate_berlin_state(bits)

    def test_hour_24_rejected(self):
        bits = "0" + "1111" + "1111" + "0" * 11 + "0000"
        with self.assertRaises(BerlinUhrError):
            validate_berlin_state(bits)

    def test_timestamp_state_mismatch(self):
        wrong = "000000000000000000000000"
        with self.assertRaises(BerlinUhrError):
            bu_validate("2026-11-01T21:00:00Z", wrong)

    def test_second_parity(self):
        even = bu_encode("2026-11-01T12:00:00Z")
        odd = bu_encode("2026-11-01T12:00:01Z")
        self.assertEqual(even[0], "0")
        self.assertEqual(odd[0], "1")
        self.assertEqual(even[1:], odd[1:])

    def test_all_kat_states_semantically_valid(self):
        for timestamp, bits in KAT:
            with self.subTest(timestamp=timestamp):
                state = validate_berlin_state(bits)
                self.assertEqual(state["hour"], int(timestamp[11:13]))
                self.assertEqual(state["minute"], int(timestamp[14:16]))


if __name__ == "__main__":
    unittest.main(verbosity=2)
