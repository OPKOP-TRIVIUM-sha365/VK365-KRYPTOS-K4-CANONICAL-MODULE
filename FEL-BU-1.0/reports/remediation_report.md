# FEL-BU-1.0 Remediation report

## Scope

This report documents the review-driven corrective actions for the identified issues REV-001 through REV-005. The work is limited to implementation, test, runner, aggregator, and documentation changes. The normative specification and canonical KAT vectors remain untouched.

## Status summary

- REV-001: Confirmed issue — execution evidence missing.
- REV-002: Confirmed issue — C parser was ad hoc and not robust enough for strict JSON validation.
- REV-003: Confirmed issue — stale exit files and run mixing risk existed.
- REV-004: Confirmed issue — README claims were insufficiently precise.
- REV-005: Confirmed issue — KAT ID uniqueness was not checked.

## Files updated

- FEL-BU-1.0/README.md
- FEL-BU-1.0/scripts/run_fel_bu_audit.sh
- FEL-BU-1.0/scripts/aggregate_conformance.py
- FEL-BU-1.0/python/test_berlin_uhr.py
- FEL-BU-1.0/rust/tests/bu_kat_tests.rs
- FEL-BU-1.0/c/bu_kat_tests.c

## What changed

### REV-001

The runtime evidence model is now explicit: results are only accepted from a dedicated run directory. The status model in the README now distinguishes between implementation presence and execution evidence. There is no PASS claim without logs and exit codes.

### REV-002

The C test file was replaced with a stricter object/array reader that validates the presence of required keys, checks the length and content of the 24-bit bitstream, validates the RGB array length and range, and rejects duplicate IDs. This is still a minimal structured parser for the known KAT JSON schema, but it is no longer a loose `strstr` fallback.

### REV-003

The runner now creates a unique run directory under `reports/runs/<run_id>/`. Each run keeps its own logs, exit-code files, and report. The aggregator reads only the active run directory and explicitly treats missing logs or exit codes as NOT_RUN.

### REV-004

The README now clearly states the difference between specification-defined, implemented, tested, executed, verified, and reviewed. The language no longer implies VERIFIED status without actual execution evidence.

### REV-005

Unique KAT ID validation was added in the Python and Rust test files. The C test also rejects duplicate IDs in the parsed corpus.

## Execution status

This environment does not contain a working shell runtime capable of executing the Python/Rust/C validation suite, so no true runtime PASS/FAIL evidence is generated here. The repository is therefore left in a remediation state, not a verified PASS state.

## Remaining risk

The specification and canonical KAT vectors remain the normative invariant. The remaining open item is actual local execution of the Python, Rust, and C test suites, as well as a real CI or local audit run from a working environment.

## Final status

Remediation state: REMEDIATION_PARTIAL

Reason: implementation, runner, aggregator, and documentation changes were made to address the identified review issues, but true runtime execution evidence remains pending because the required toolchain is not available in this environment.
