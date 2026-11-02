# FEL-BU-1.0
# Canonical UTC Berlin-Uhr Projection

This repository contains the audit-ready specification, canonical KAT vectors, and reference implementations for the Berlin-Uhr projection.

[![FEL-BU-1.0 Audit](https://img.shields.io/badge/status-audit--ready-blue)](#)
[![CI](https://img.shields.io/badge/CI-local%20%2B%20GitHub%20Actions-orange)](#)

## Scope

This profile defines a canonical Berlin-Uhr bit sequence derived strictly from the canonical UTC timestamp.

Normative rule:
- B = BU(T)

The Berlin-Uhr bit sequence is not an implementation-defined state. It is always derived from the canonical timestamp.

This yields:
- T -> BU(T) -> C(B) -> TWR -> TH -> FTTHR

Important:
- SHA365-AGG-1.0 remains unchanged.
- KAT-001 remains unchanged.
- Gregorian year range is normative: 0001..9999.

## Normative profile

Profile ID:
- FEL-BU-1.0

Profile name:
- Canonical UTC Berlin-Uhr Projection

## File layout

- specification/FEL-BU-1.0.md
- vectors/bu_kat_001.json
- python/berlin_uhr.py
- python/test_berlin_uhr.py
- rust/Cargo.toml
- rust/tests/bu_kat_tests.rs
- c/bu_kat_tests.c
- scripts/run_fel_bu_audit.sh
- scripts/aggregate_conformance.py
- scripts/create_archive_manifest.py
- .github/workflows/fel-bu-1.0.yml
- reports/conformance_report.json
- reports/audit_summary.md
- artifacts/archive_manifest.json

## Local execution

Python:
```bash
cd python
python -m unittest -v test_berlin_uhr.py
```

Rust:
```bash
cd rust
cargo test -- --nocapture
```

C:
```bash
cd c
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 bu_kat_tests.c -o bu_kat_tests
./bu_kat_tests
```

Full audit bundle:
```bash
bash scripts/run_fel_bu_audit.sh
```

Or using make:
```bash
make audit
```

Generate archive manifest:
```bash
make manifest
```

## Result policy

- PASS requires all KAT vectors, CMAP RGB/HEX checks, and negative tests to pass in all implementations.
- FAIL requires any mismatch, invalid accepted value, rejected valid input, or wrong CMAP derivation.
- Status in reports/conformance_report.json must be updated only after real local execution.
- CI may validate the same conditions, but local execution remains the source of final truth.

## Execution note

This repository is audit-ready for local execution, but real PASS/FAIL status is not assigned in this session because code is not executed here.
