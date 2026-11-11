#!/usr/bin/env python3
import hashlib
import json
import platform
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
reports = root / "reports"

def read_exit(name):
    try:
        return int((reports / f"{name}.exit").read_text(encoding="utf-8").strip())
    except (OSError, ValueError):
        return None

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None

def missing(paths):
    return [str(p.relative_to(root)) for p in paths if not p.is_file()]

rust = read_exit("rust")
c = read_exit("c")
fixture_validation = read_exit("fixture_validation")
required = [reports / name for name in (
    "fixture_validation.log", "fixture_validation.exit", "rust.log", "rust.exit",
    "rust_build.log", "rust_build.exit", "c.log", "c.exit", "c_build.log",
    "c_build.exit")]
missing_outputs = missing(required)
if missing_outputs:
    status = "NOT_RUN"
elif fixture_validation != 0:
    status = "FAIL"
elif rust == 0 and c == 0:
    status = "PASS"
elif rust in (None, 127) or c in (None, 127):
    status = "NOT_RUN"
else:
    status = "FAIL"

def lang_status(code):
    if code == 0:
        return "PASS"
    if code in (None, 127, 125):
        return "NOT_RUN"
    return "FAIL"

report = {
    "profile": "FEL-BU-1.0 Rust/C shared KAT conformance",
    "fixture": "fixtures/bu_kat_001.tsv",
    "fixture_sha256": digest(root / "fixtures" / "bu_kat_001.tsv"),
    "fixture_status": "PASS" if fixture_validation == 0 else ("FAIL" if fixture_validation not in (None, 127) else "NOT_RUN"),
    "overall_rust_c_status": status,
    "rust_status": lang_status(rust),
    "c_status": lang_status(c),
    "rust_exit_code": rust,
    "c_exit_code": c,
    "rust_build_exit_code": read_exit("rust_build"),
    "c_build_exit_code": read_exit("c_build"),
    "fixture_validation_exit_code": fixture_validation,
    "missing_execution_artifacts": missing_outputs,
    "rust_log_sha256": digest(reports / "rust.log"),
    "c_log_sha256": digest(reports / "c.log"),
    "rust_build_log_sha256": digest(reports / "rust_build.log"),
    "c_build_log_sha256": digest(reports / "c_build.log"),
    "fixture_validation_log_sha256": digest(reports / "fixture_validation.log"),
    "host_platform": platform.platform(),
    "python": sys.version.split()[0],
    "scope_note": "PASS applies only to these Rust and C implementations against the validated shared BU/CMAP KAT fixture. It is not repository-wide verification, CI evidence, or verification of TWR/TH/FTTHR.",
    "verified_status": "NOT_VERIFIED",
}
(reports / "CONFORMANCE_REPORT.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print("Aggregate Rust/C status:", status)
if missing_outputs:
    print("Missing evidence files:", ", ".join(missing_outputs))
sys.exit(0 if status == "PASS" else 1)
