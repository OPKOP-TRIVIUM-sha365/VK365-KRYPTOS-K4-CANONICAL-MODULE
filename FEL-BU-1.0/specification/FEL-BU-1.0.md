# FEL-BU-1.0
# Canonical UTC Berlin-Uhr Projection

## 1. Purpose

This profile defines a canonical Berlin-Uhr bit encoding derived strictly from the canonical UTC timestamp.

The key normative rule is:

B = BU(T)

The Berlin-Uhr bit sequence is not an implementation-defined state. It is always computed from the canonical timestamp.

This yields:

T -> BU(T) -> C(B) -> TWR -> TH -> FTTHR

Where:
- T = canonical UTC timestamp
- BU(T) = Berlin-Uhr bit vector
- C(B) = color mapping from Berlin state
- TWR = derived time representation
- TH = derived time hash
- FTTHR = final transformed hash representation

SHA365-AGG-1.0 and KAT-001 remain unchanged.

---

## 2. Normative definition of BU(T)

### 2.1 Input format

Timestamp must be an ASCII string in the exact form:

YYYY-MM-DDTHH:MM:SSZ

Normative constraints:
- Gregorian calendar
- UTC only, suffix Z
- exact 20 ASCII characters
- no fractional seconds
- no timezone offsets such as +01:00
- no leap-second notation :60
- year in 0001..9999
- hour in 00..23
- minute in 00..59
- second in 00..59

Example:
- 2026-11-01T21:00:00Z

### 2.2 24-bit lamp layout

The logical 24-bit Berlin-Uhr sequence is:

- bit 1: second parity indicator
- row 1: five-hour lamps
- row 2: one-hour lamps
- row 3: five-minute lamps
- row 4: one-minute lamps

All rows use prefix-ON encoding:
- leading lamps are ON
- remaining lamps are OFF

### 2.3 Mathematical equations

For timestamp T = (h, m, s):

Second parity:
- b1 = s mod 2

Hour rows:
- H5 = floor(h / 5)
- H1 = h mod 5

Minute rows:
- M5 = floor(m / 5)
- M1 = m mod 5

Therefore:

BU(T) = Parity(s) || Prefix_4(H5) || Prefix_4(H1) || Prefix_11(M5) || Prefix_4(M1)

Total bit length:
- 1 + 4 + 4 + 11 + 4 = 24 bits

---

## 3. Reference validator

### 3.1 Canonical validation rules

The implementation must:
1. verify the timestamp is a string
2. verify the timestamp is ASCII
3. verify exact format and length
4. verify Gregorian calendar validity
5. verify year range 0001..9999
6. verify UTC syntax and suffix Z
7. verify 24-bit Berlin-Uhr state length
8. verify prefix-ON validity in every row
9. verify decoded hour and minute ranges
10. verify exact equality between generated Berlin state and expected state

### 3.2 Structural validity

A Berlin state is structurally valid only if:
- it contains exactly 24 bits
- every character is '0' or '1'
- each row is prefix-ON
- no row contains the pattern "01"

### 3.3 Temporal decoding

Given a 24-bit state:
- second bit = bits[0]
- five-hour row = bits[1:5]
- one-hour row = bits[5:9]
- five-minute row = bits[9:20]
- one-minute row = bits[20:24]

Then:
- h = 5 * count("1" in five-hour row) + count("1" in one-hour row)
- m = 5 * count("1" in five-minute row) + count("1" in one-minute row)

Requirements:
- h must be in 0..23
- m must be in 0..59

The second bit records only parity, not the exact second value. This is important for validation design.

---

## 4. Normative KAT vectors

### 4.1 BU-KAT-001

These five vectors define the canonical Berlin-Uhr state for UTC timestamps.

| ID | Timestamp | Expected BU bits |
|---|---|---|
| BU-KAT-001-A | 2026-11-01T00:00:00Z | 000000000000000000000000 |
| BU-KAT-001-B | 1990-11-03T05:58:00Z | 010000000111111111111110 |
| BU-KAT-001-C | 2026-10-24T08:26:00Z | 010001110111110000001000 |
| BU-KAT-001-D | 2026-11-01T21:00:00Z | 011111000000000000000000 |
| BU-KAT-001-E | 2026-11-01T23:59:59Z | 111111110111111111111111 |

These vectors are normative for all implementations.

---

## 5. CMAP projection

For Berlin state B, RGB is derived using integer arithmetic:

- R_s = int(B[0:9], 2)
- G_s = int(B[9:20], 2)
- B_s = int(B[20:24], 2)

Then:

- R = (R_s * 255) // 511
- G = (G_s * 255) // 2047
- B = (B_s * 255) // 15

Here // is integer division for non-negative operands.

The hex output is uppercase hexadecimal derived from the RGB triplet.

Normative KAT values:
- BU-KAT-001-A -> RGB = [0, 0, 0], HEX = 000000
- BU-KAT-001-B -> RGB = [63, 255, 238], HEX = 3FFFEE
- BU-KAT-001-C -> RGB = [70, 247, 136], HEX = 46F788
- BU-KAT-001-D -> RGB = [123, 0, 0], HEX = 7B0000
- BU-KAT-001-E -> RGB = [254, 255, 255], HEX = FEFFFF

---

## 6. Normative rules summary

- FEL-BU-001: Timestamp is the canonical UTC source
- FEL-BU-002: second bit is second % 2
- FEL-BU-003: hour and minute rows use prefix-ON
- FEL-BU-004: year is in 0001..9999
- FEL-BU-005: hour is in 0..23
- FEL-BU-006: minute is in 0..59
- FEL-BU-007: Berlin state has exactly 24 bits
- FEL-BU-008: CMAP uses integer division
- FEL-BU-009: CMAP uses R_s/G_s/B_s slices from B
- FEL-BU-010: HEX is uppercase and derived from RGB
- FEL-BU-011: TWR preserves the original Berlin state
- FEL-BU-012: SHA365-AGG-1.0 remains unchanged

---

## 7. Execution status

This specification is defined and stable.

Actual execution of code remains pending until it is run in a local runtime environment.

This document is authoritative for the canonical model and the KAT corpus.
