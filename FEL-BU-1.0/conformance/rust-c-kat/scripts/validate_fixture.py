#!/usr/bin/env python3
"""Validate the shared fixture's exact structure and canonical KAT rows."""
import sys
from pathlib import Path

EXPECTED = [
    "A\t2026-11-01T00:00:00Z\t000000000000000000000000\t0,0,0\t000000",
    "B\t1990-11-03T05:58:00Z\t010000000111111111111110\t63,255,238\t3FFFEE",
    "C\t2026-10-24T08:26:00Z\t010001110111110000001000\t70,247,136\t46F788",
    "D\t2026-11-01T21:00:00Z\t011111000000000000000000\t123,0,0\t7B0000",
    "E\t2026-11-01T23:59:59Z\t111111110111111111111111\t254,255,255\tFEFFFF",
]
root = Path(__file__).resolve().parents[1]
path = root / "fixtures" / "bu_kat_001.tsv"
try:
    raw = path.read_bytes()
    text = raw.decode("ascii")
except (OSError, UnicodeDecodeError) as exc:
    print(f"FAIL fixture read/ASCII: {exc}", file=sys.stderr)
    raise SystemExit(1)
lines = text.splitlines()
if not lines or lines[0] != "# id\ttimestamp\tbits\trgb\thex":
    print("FAIL fixture header or TAB delimiters", file=sys.stderr)
    raise SystemExit(1)
rows = lines[1:]
if rows != EXPECTED:
    print("FAIL fixture rows differ from canonical five-row corpus", file=sys.stderr)
    raise SystemExit(1)
if any(len(row.split("\t")) != 5 for row in rows):
    print("FAIL fixture row field count", file=sys.stderr)
    raise SystemExit(1)
print("PASS fixture structure, TAB delimiters, and five canonical rows")
