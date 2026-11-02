#!/usr/bin/env python3
import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


def read_exit_code(path: Path):
    if not path.exists():
        return None
    try:
        value = path.read_text(encoding="utf-8", errors="replace").strip()
        if value == "":
            return None
        return int(value)
    except ValueError:
        return None


def sha256_file(path: Path):
    if not path.exists():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def component_status(exit_code):
    if exit_code is None:
        return "NOT_RUN"
    if exit_code == 0:
        return "PASS"
    return "FAIL"


def build_summary(report):
    lines = []
    lines.append("# FEL-BU-1.0 Audit Summary")
    lines.append("")
    lines.append(f"Run ID: {report.get('run_id', 'unknown')}")
    lines.append(f"Overall status: {report.get('status', 'PENDING_LOCAL_EXECUTION')}")
    lines.append("")
    lines.append("## Component status")
    for name in ("python", "rust", "c"):
        comp = report[name]
        lines.append(f"- {name}: {comp['status']} (exit_code={comp.get('exit_code')})")
    lines.append("")
    lines.append("## Notes")
    lines.append("- This report reflects only the active run directory.")
    lines.append("- PASS requires an actual execution and exit code 0 for all required components.")
    lines.append("- Missing log or exit file yields NOT_RUN. Missing file is never treated as PASS.")
    return "\n".join(lines) + "\n"


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--root", required=True)
    p.add_argument("--run-dir", required=True)
    p.add_argument("--report", required=True)
    p.add_argument("--summary", required=True)
    args = p.parse_args()

    root = Path(args.root).resolve()
    run_dir = Path(args.run_dir).resolve()
    report_path = Path(args.report).resolve()
    summary_path = Path(args.summary).resolve()

    metadata = {}
    metadata_path = run_dir / "metadata.json"
    if metadata_path.exists():
        try:
            metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        except Exception:
            metadata = {}

    commands = {
        "python": "cd python && python3 -m unittest -v test_berlin_uhr.py",
        "rust": "cd rust && cargo test -- --nocapture",
        "c": "cd c && gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 bu_kat_tests.c -o bu_kat_tests && ./bu_kat_tests",
    }

    results = {}
    for name in ("python", "rust", "c"):
        log_path = run_dir / f"{name}.log"
        exit_path = run_dir / f"{name}.exit"
        exit_code = read_exit_code(exit_path)

        results[name] = {
            "name": name,
            "status": component_status(exit_code),
            "exit_code": exit_code,
            "command": commands[name],
            "log": str(log_path.name),
            "sha256_log": sha256_file(log_path),
            "run_id": metadata.get("run_id", run_dir.name),
        }

    statuses = [results[name]["status"] for name in ("python", "rust", "c")]
    if any(s == "FAIL" for s in statuses):
        overall_status = "FAIL"
    elif all(s == "PASS" for s in statuses):
        overall_status = "PASS"
    else:
        overall_status = "PENDING_LOCAL_EXECUTION"

    report = {
        "profile": "FEL-BU-1.0",
        "suite": "BU-KAT-001",
        "run_id": metadata.get("run_id", run_dir.name),
        "generated_at_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "status": overall_status,
        "python": results["python"],
        "rust": results["rust"],
        "c": results["c"],
        "notes": [
            "This report reflects only the active run directory and does not combine data from older runs.",
            "PASS requires actual execution and exit code 0 for all required components.",
            "A missing log or exit file yields NOT_RUN and prevents PASS.",
            "A non-zero exit code yields FAIL."
        ],
    }

    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    summary_path.write_text(build_summary(report), encoding="utf-8")

    print(json.dumps(report, indent=2))
    return 0 if overall_status == "PASS" else 1 if overall_status == "FAIL" else 0


if __name__ == "__main__":
    raise SystemExit(main())
