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
        if not value:
            return None
        return int(value)
    except ValueError:
        return None


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalize_exit_status(exit_code):
    if exit_code is None:
        return "NOT_RUN"
    return "PASS" if exit_code == 0 else "FAIL"


def collect_component(name, run_dir: Path, command: str):
    log_path = run_dir / f"{name}.log"
    exit_path = run_dir / f"{name}.exit"
    exit_code = read_exit_code(exit_path)
    log_hash = sha256_file(log_path) if log_path.exists() else None
    return {
        "name": name,
        "status": normalize_exit_status(exit_code),
        "exit_code": exit_code,
        "command": command,
        "log": str(log_path.name),
        "sha256_log": log_hash,
    }


def summary_text(report):
    lines = []
    lines.append("# FEL-BU-1.0 Audit Summary")
    lines.append("")
    lines.append(f"Run ID: {report.get('run_id', 'unknown')}")
    lines.append(f"Overall status: {report['status']}")
    lines.append("")
    lines.append("## Component status")
    for name in ("python", "rust", "c"):
        comp = report[name]
        lines.append(f"- {name}: {comp['status']} (exit_code={comp.get('exit_code')})")
    lines.append("")
    lines.append("## Notes")
    lines.append("- This summary reflects only the active run directory and does not combine older results.")
    lines.append("- PASS requires an actual execution and exit code 0 for all three implementations.")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    parser.add_argument("--run-dir", required=True)
    parser.add_argument("--report", required=True)
    parser.add_argument("--summary", required=True)
    args = parser.parse_args()

    root = Path(args.root).resolve()
    run_dir = Path(args.run_dir).resolve()
    report_path = Path(args.report).resolve()
    summary_path = Path(args.summary).resolve()

    metadata_path = run_dir / "metadata.json"
    metadata = {}
    if metadata_path.exists():
        try:
            metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        except Exception:
            metadata = {}

    python_cmd = "cd python && python3 -m unittest -v test_berlin_uhr.py"
    rust_cmd = "cd rust && cargo test -- --nocapture"
    c_cmd = "cd c && gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 bu_kat_tests.c -o bu_kat_tests && ./bu_kat_tests"

    results = {
        "python": collect_component("python", run_dir, python_cmd),
        "rust": collect_component("rust", run_dir, rust_cmd),
        "c": collect_component("c", run_dir, c_cmd),
    }

    statuses = [v["status"] for v in results.values()]
    if any(v == "FAIL" for v in statuses):
        overall_status = "FAIL"
    elif all(v == "PASS" for v in statuses):
        overall_status = "PASS"
    else:
        overall_status = "PENDING_LOCAL_EXECUTION"

    payload = {
        "profile": "FEL-BU-1.0",
        "suite": "BU-KAT-001",
        "run_id": metadata.get("run_id", run_dir.name),
        "generated_at_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "status": overall_status,
        "python": results["python"],
        "rust": results["rust"],
        "c": results["c"],
        "notes": [
            "This report reflects only the active run directory and does not include stale results from other runs.",
            "PASS requires an actual execution and exit code 0 for each component.",
            "FAIL requires a non-zero exit code or a rejected validation condition.",
            "A missing exit code or missing log is treated as NOT_RUN."
        ],
    }

    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    summary_path.write_text(summary_text(payload), encoding="utf-8")

    print(json.dumps(payload, indent=2))
    return 0 if overall_status == "PASS" else 1 if overall_status == "FAIL" else 0


if __name__ == "__main__":
    raise SystemExit(main())
