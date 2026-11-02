#!/usr/bin/env python3
import argparse
import json
from pathlib import Path
from datetime import datetime, timezone


def read_exit_code(path: Path):
    if not path.exists():
        return None
    try:
        return int(path.read_text(encoding="utf-8", errors="replace").strip())
    except ValueError:
        return None


def normalize_status(exit_code):
    if exit_code is None:
        return "NOT_RUN"
    return "PASS" if exit_code == 0 else "FAIL"


def collect_result(name, command, log_path, exit_code, root):
    relative_log = log_path.relative_to(root) if log_path.is_absolute() else log_path
    return {
        "name": name,
        "status": normalize_status(exit_code),
        "exit_code": exit_code,
        "command": command,
        "log": str(relative_log),
    }


def build_summary(report):
    overall = report["status"]
    lines = []
    lines.append("# FEL-BU-1.0 Audit Summary")
    lines.append("")
    lines.append(f"Overall status: {overall}")
    lines.append("")
    lines.append("## Component status")
    for lang in ("python", "rust", "c"):
        item = report[lang]
        lines.append(f"- {lang}: {item['status']} (exit_code={item.get('exit_code')})")
    lines.append("")
    lines.append("## Notes")
    lines.append("- This summary is generated after executing the local audit suite.")
    lines.append("- PASS means all KAT vectors, CMAP RGB/HEX checks, and negative tests passed.")
    lines.append("- FAIL means at least one KAT vector, CMAP value, or validation path failed.")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    parser.add_argument("--report", required=True)
    parser.add_argument("--summary", required=True)
    args = parser.parse_args()

    root = Path(args.root).resolve()
    report_path = Path(args.report).resolve()
    summary_path = Path(args.summary).resolve()

    report_dir = report_path.parent
    report_dir.mkdir(parents=True, exist_ok=True)

    python_exit = read_exit_code(root / "reports" / "python.exit")
    rust_exit = read_exit_code(root / "reports" / "rust.exit")
    c_exit = read_exit_code(root / "reports" / "c.exit")

    python_cmd = "cd python && python -m unittest -v test_berlin_uhr.py"
    rust_cmd = "cd rust && cargo test -- --nocapture"
    c_cmd = "cd c && gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 bu_kat_tests.c -o bu_kat_tests && ./bu_kat_tests"

    results = {
        "python": collect_result("python", python_cmd, root / "reports" / "python.log", python_exit, root),
        "rust": collect_result("rust", rust_cmd, root / "reports" / "rust.log", rust_exit, root),
        "c": collect_result("c", c_cmd, root / "reports" / "c.log", c_exit, root),
    }

    if any(item["status"] == "FAIL" for item in results.values()):
        overall_status = "FAIL"
    elif any(item["status"] == "NOT_RUN" for item in results.values()):
        overall_status = "PENDING_LOCAL_EXECUTION"
    else:
        overall_status = "PASS"

    report = {
        "profile": "FEL-BU-1.0",
        "suite": "BU-KAT-001",
        "generated_at_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "status": overall_status,
        "execution_policy": "No final verdict is assigned until code is run in a local runtime or CI environment.",
        "python": results["python"],
        "rust": results["rust"],
        "c": results["c"],
        "result_policy": {
            "pass": "All KAT vectors, CMAP RGB/HEX values, and negative tests pass in all implementations.",
            "fail": "Any mismatch, invalid accepted value, rejected valid input, or wrong CMAP derivation counts as FAIL.",
            "final_status": "SET_ONLY_AFTER_LOCAL_OR_CI_EXECUTION"
        }
    }

    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    summary_path.write_text(build_summary(report), encoding="utf-8")

    print(json.dumps(report, indent=2))
    print("")
    print(f"Summary written to: {summary_path}")


if __name__ == "__main__":
    main()
