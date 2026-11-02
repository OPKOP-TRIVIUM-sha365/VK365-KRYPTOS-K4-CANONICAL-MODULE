# FEL-BU-1.0 Remediation Report

## Scope

This report documents the remediation effort for review findings REV-001 through REV-005.

Important:
- The normative specification remains unchanged.
- The canonical KAT vectors remain unchanged.
- This report documents implementation and process corrections only.
- Runtime verification remains pending until actual logs and exit codes exist.

## Status summary

| Finding | Status | Summary |
| --- | --- | --- |
| REV-001 | PENDING_EXECUTION | Execution evidence missing |
| REV-002 | PENDING_EXECUTION | C JSON parser must be replaced with strict parser |
| REV-003 | PENDING_EXECUTION | Run isolation and stale results risk must be fixed |
| REV-004 | PENDING_EXECUTION | README and reports need precise status language |
| REV-005 | PENDING_EXECUTION | KAT ID uniqueness validation missing |

## REV-001 — Execution evidence

Original issue:
- The repository contains implementation files and tests, but no real execution evidence.

Files targeted:
- README.md
- scripts/run_fel_bu_audit.sh
- scripts/aggregate_conformance.py

Changes made:
- Added unique run_id behavior.
- Added metadata generation.
- Added per-language logs and exit codes.
- Added aggregate report generation from active run directory only.

Remaining risk:
- Actual runtime execution still required.

## REV-002 — C JSON parser

Original issue:
- C test used ad hoc text extraction instead of robust JSON parsing.

Files targeted:
- c/bu_kat_tests.c

Changes made:
- Removed reliance on ad hoc string extraction as the primary parser.
- Added strict JSON parsing using a proper C JSON library or equivalent validated parser.
- Added validation of root object, vectors array, required fields, types, ranges, and duplicate IDs.

Remaining risk:
- Runtime execution still required to confirm parser behavior.

## REV-003 — Run isolation / stale results

Original issue:
- Aggregator may read stale exit-code files or mismatched run outputs.

Files targeted:
- scripts/run_fel_bu_audit.sh
- scripts/aggregate_conformance.py

Changes made:
- Added per-run directory under reports/runs/<RUN_ID>.
- Ensured logs and exit files are created for each run.
- Ensured aggregator reads only the active run directory.
- Added explicit handling for missing log or missing exit file.

Remaining risk:
- Real execution still required to generate current artifacts.

## REV-004 — README and report precision

Original issue:
- README and reports implied verified status without actual evidence.

Files targeted:
- README.md

Changes made:
- Added explicit status vocabulary.
- Distinction between implementation presence and execution evidence.
- No PASS/VERIFIED claim without logs and exit codes.

Remaining risk:
- Execution evidence still pending.

## REV-005 — KAT ID uniqueness

Original issue:
- KAT IDs were not validated for uniqueness.

Files targeted:
- python/test_berlin_uhr.py
- rust/tests/bu_kat_tests.rs
- c/bu_kat_tests.c

Changes made:
- Added uniqueness check logic in Python, Rust and C tests.
- Added negative duplicate-ID test patterns.
- Added validation that the canonical KAT has exactly one entry per ID.

Remaining risk:
- Requires actual execution to confirm.
