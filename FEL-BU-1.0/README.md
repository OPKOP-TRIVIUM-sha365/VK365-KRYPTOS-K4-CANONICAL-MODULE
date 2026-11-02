# FEL-BU-1.0

Canonical UTC Berlin-Uhr Projection

## Status model

This package distinguishes between implementation state and execution evidence.

- SPECIFICATION_DEFINED: the profile, formulas, and canonical KAT corpus are present and stable.
- IMPLEMENTATION_PRESENT: Python, Rust and C sources exist.
- TESTS_PRESENT: positive and negative tests are present.
- TESTS_EXECUTED: a runtime command actually ran and produced evidence.
- CROSS_PLATFORM_CONFORMANCE_VERIFIED: Python, Rust and C all executed successfully against the same corpus.
- CI_VERIFIED: GitHub Actions actually ran with artifacts attached.
- INDEPENDENT_REVIEWED: review confirmed no contradictory findings after evidence review.

Important:
- PASS, VERIFIED, and CERTIFIED claims require actual execution evidence.
- If evidence is absent, the status must remain NOT_RUN or PENDING_EXECUTION.
- This repository contains implementation scaffolding and tests, but it does not claim verified runtime PASS without logs and exit-code evidence.

## Normative profile

Profile ID: FEL-BU-1.0
Profile name: Canonical UTC Berlin-Uhr Projection
Suite: BU-KAT-001

Important:
- SHA365-AGG-1.0 and KAT-001 remain unchanged.
- The canonical KAT values are the reference for all implementations.
- Any PASS or VERIFIED claim must be backed by a real execution record.

## Repository structure

- specification/FEL-BU-1.0.md
- vectors/bu_kat_001.json
- python/berlin_uhr.py
- python/test_berlin_uhr.py
- rust/Cargo.toml
- rust/tests/bu_kat_tests.rs
- c/bu_kat_tests.c
- scripts/run_fel_bu_audit.sh
- scripts/aggregate_conformance.py
- .github/workflows/fel-bu-1.0.yml
- reports/runs/
- reports/remediation_report.md

## Local commands

Python:
```bash
cd FEL-BU-1.0/python
python3 -m unittest -v test_berlin_uhr.py
```

Rust:
```bash
cd FEL-BU-1.0/rust
cargo test -- --nocapture
```

C:
```bash
cd FEL-BU-1.0/c
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 bu_kat_tests.c -o bu_kat_tests
./bu_kat_tests
```

Audit runner:
```bash
bash FEL-BU-1.0/scripts/run_fel_bu_audit.sh
```

Make targets:
```bash
make -C FEL-BU-1.0 python
make -C FEL-BU-1.0 rust
make -C FEL-BU-1.0 c
make -C FEL-BU-1.0 audit
```

## Execution audit policy

- Absence of actual logs or exit codes means NOT_RUN, not PASS.
- Any non-zero exit code means FAIL.
- A PASS result requires each implementation to be executed and to return exit code 0.
- A stale run result is not accepted as evidence of the current run.
- The review process distinguishes between implementation presence and runtime verification.

## Current status

The implementation and tests are present in the repository, but runtime verification is not claimed here unless a real log and exit-code record exists in the active run directory.
