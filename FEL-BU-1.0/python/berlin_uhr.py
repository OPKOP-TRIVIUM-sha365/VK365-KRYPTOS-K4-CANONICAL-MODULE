#!/usr/bin/env python3
"""
FEL-BU-1.0
Canonical UTC Berlin-Uhr Projection
"""

from __future__ import annotations
from datetime import datetime
import json
import re
from pathlib import Path

TIMESTAMP_RE = re.compile(
    r"^[0-9]{4}-[0-9]{2}-[0-9]{2}"
    r"T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$"
)


class BerlinUhrError(ValueError):
    pass


def parse_timestamp(timestamp: str) -> datetime:
    if not isinstance(timestamp, str):
        raise BerlinUhrError("TIMESTAMP_TYPE")
    if not timestamp.isascii():
        raise BerlinUhrError("TIMESTAMP_NON_ASCII")
    if len(timestamp) != 20:
        raise BerlinUhrError("TIMESTAMP_LENGTH")
    if not TIMESTAMP_RE.fullmatch(timestamp):
        raise BerlinUhrError("TIMESTAMP_FORMAT")

    year = int(timestamp[0:4])
    if year < 1 or year > 9999:
        raise BerlinUhrError("TIMESTAMP_YEAR_RANGE")

    try:
        return datetime.strptime(timestamp, "%Y-%m-%dT%H:%M:%SZ")
    except ValueError as exc:
        raise BerlinUhrError("TIMESTAMP_VALUE") from exc


def prefix_on(count: int, width: int) -> str:
    if type(count) is not int:
        raise BerlinUhrError("COUNT_TYPE")
    if not 0 <= count <= width:
        raise BerlinUhrError("COUNT_RANGE")
    return "1" * count + "0" * (width - count)


def bu_encode(timestamp: str) -> str:
    dt = parse_timestamp(timestamp)
    h = dt.hour
    m = dt.minute
    s = dt.second

    second_bit = str(s % 2)
    five_hour = prefix_on(h // 5, 4)
    one_hour = prefix_on(h % 5, 4)
    five_minute = prefix_on(m // 5, 11)
    one_minute = prefix_on(m % 5, 4)

    result = second_bit + five_hour + one_hour + five_minute + one_minute
    if len(result) != 24:
        raise BerlinUhrError("INTERNAL_LENGTH")
    return result


def validate_berlin_state(bits: str) -> dict:
    if not isinstance(bits, str):
        raise BerlinUhrError("BERLIN_TYPE")
    if len(bits) != 24:
        raise BerlinUhrError("BERLIN_LENGTH")
    if any(bit not in "01" for bit in bits):
        raise BerlinUhrError("BERLIN_BITS")

    second_bit = bits[0]
    five_hour = bits[1:5]
    one_hour = bits[5:9]
    five_minute = bits[9:20]
    one_minute = bits[20:24]

    rows = (
        ("FIVE_HOUR", five_hour),
        ("ONE_HOUR", one_hour),
        ("FIVE_MINUTE", five_minute),
        ("ONE_MINUTE", one_minute),
    )

    for name, row in rows:
        if "01" in row:
            raise BerlinUhrError("NON_PREFIX_ROW:" + name)

    h = 5 * five_hour.count("1") + one_hour.count("1")
    m = 5 * five_minute.count("1") + one_minute.count("1")

    if h > 23:
        raise BerlinUhrError("HOUR_OUT_OF_RANGE")
    if m > 59:
        raise BerlinUhrError("MINUTE_OUT_OF_RANGE")

    return {
        "second_parity": int(second_bit),
        "hour": h,
        "minute": m,
    }


def bu_validate(timestamp: str, bits: str) -> bool:
    expected = bu_encode(timestamp)
    validate_berlin_state(bits)
    if bits != expected:
        raise BerlinUhrError("TIMESTAMP_BERLIN_MISMATCH")
    return True


def cmap_rgb_from_bits(bits: str) -> tuple[int, int, int]:
    validate_berlin_state(bits)
    r_s = int(bits[0:9], 2)
    g_s = int(bits[9:20], 2)
    b_s = int(bits[20:24], 2)

    r = (r_s * 255) // 511
    g = (g_s * 255) // 2047
    b = (b_s * 255) // 15
    return r, g, b


def cmap_hex_from_bits(bits: str) -> str:
    r, g, b = cmap_rgb_from_bits(bits)
    return f"{r:02X}{g:02X}{b:02X}"


def bu_vector(timestamp: str) -> dict:
    bits = bu_encode(timestamp)
    state = validate_berlin_state(bits)
    r, g, b = cmap_rgb_from_bits(bits)
    return {
        "profile": "FEL-BU-1.0",
        "timestamp": timestamp,
        "berlin_bits": bits,
        "bit_length": len(bits),
        "decoded_hour": state["hour"],
        "decoded_minute": state["minute"],
        "second_parity": state["second_parity"],
        "rgb": [r, g, b],
        "hex": cmap_hex_from_bits(bits),
        "status": "VALID",
    }


def load_kat_vectors() -> list[tuple[str, str, list[int], str]]:
    data_path = Path(__file__).resolve().parents[1] / "vectors" / "bu_kat_001.json"
    with open(data_path, "r", encoding="utf-8") as fh:
        payload = json.load(fh)
    vectors = []
    for item in payload["vectors"]:
        vectors.append((item["timestamp"], item["berlin"], item["rgb"], item["hex"]))
    return vectors
