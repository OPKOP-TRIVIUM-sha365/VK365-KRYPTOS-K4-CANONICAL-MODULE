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
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.exists() else None

rust = read_exit("rust")
c = read_exit("c")
status = "PASS" if rust == 0 and c == 0 else (
    "FAIL" if (rust not in (None, 0, 127) or c not in (None, 0, 127)) else "NOT_RUN")
report = {
    "profile": "FEL-BU-1.0 Rust/C shared KAT conformance",
    "fixture": "fixtures/bu_kat_001.tsv",
    "fixture_sha256": digest(root / "fixtures/bu_kat_001.tsv"),
    "overall_rust_c_status": status,
    "rust_status": "PASS" if rust == 0 else ("NOT_RUN" if rust in (None, 127) else "FAIL"),
    "c_status": "PASS" if c == 0 else ("NOT_RUN" if c in (None, 127) else "FAIL"),
    "rust_exit_code": rust,
    "c_exit_code": c,
    "rust_log_sha256": digest(reports / "rust.log"),
    "c_log_sha256": digest(reports / "c.log"),
    "rust_build_log_sha256": digest(reports / "rust_build.log"),
    "c_build_log_sha256": digest(reports / "c_build.log"),
    "host_platform": platform.platform(),
    "python": sys.version.split()[0],
    "scope_note": "PASS applies only to these Rust and C implementations against the shared BU/CMAP KAT fixture. It is not repository-wide verification, CI evidence, or verification of TWR/TH/FTTHR.",
    "verified_status": "NOT_VERIFIED",
}
(reports / "CONFORMANCE_REPORT.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print("Aggregate Rust/C status:", status)
sys.exit(0 if status == "PASS" else 1)
