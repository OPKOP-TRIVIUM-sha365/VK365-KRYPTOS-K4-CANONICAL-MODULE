#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
REPORT_DIR="$ROOT_DIR/reports"
mkdir -p "$REPORT_DIR"

PYTHON_LOG="$REPORT_DIR/python.log"
RUST_LOG="$REPORT_DIR/rust.log"
C_LOG="$REPORT_DIR/c.log"

PYTHON_EXIT="$REPORT_DIR/python.exit"
RUST_EXIT="$REPORT_DIR/rust.exit"
C_EXIT="$REPORT_DIR/c.exit"
AGG_EXIT="$REPORT_DIR/aggregate.exit"

rm -f "$PYTHON_LOG" "$RUST_LOG" "$C_LOG"
rm -f "$PYTHON_EXIT" "$RUST_EXIT" "$C_EXIT" "$AGG_EXIT"

run_step() {
  local label="$1"
  local command="$2"
  local log_file="$3"
  local exit_file="$4"

  bash -lc "$command" >"$log_file" 2>&1
  local status=$?
  printf "%s" "$status" >"$exit_file"
  echo "[$label] exit_code=$status"
}

run_step "python" "cd \"$ROOT_DIR/python\" && python -m unittest -v test_berlin_uhr.py" "$PYTHON_LOG" "$PYTHON_EXIT"
run_step "rust" "cd \"$ROOT_DIR/rust\" && cargo test -- --nocapture" "$RUST_LOG" "$RUST_EXIT"
run_step "c" "cd \"$ROOT_DIR/c\" && gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 bu_kat_tests.c -o bu_kat_tests && ./bu_kat_tests" "$C_LOG" "$C_EXIT"

if ! python3 "$SCRIPT_DIR/aggregate_conformance.py" \
  --root "$ROOT_DIR" \
  --report "$REPORT_DIR/conformance_report.json" \
  --summary "$REPORT_DIR/audit_summary.md"; then
  echo "[aggregate] failed"
  printf "%s" "1" >"$AGG_EXIT"
  exit 1
fi

printf "%s" "0" >"$AGG_EXIT"

overall_status="PASS"
if [ -f "$PYTHON_EXIT" ] && [ "$(cat "$PYTHON_EXIT")" != "0" ]; then overall_status="FAIL"; fi
if [ -f "$RUST_EXIT" ] && [ "$(cat "$RUST_EXIT")" != "0" ]; then overall_status="FAIL"; fi
if [ -f "$C_EXIT" ] && [ "$(cat "$C_EXIT")" != "0" ]; then overall_status="FAIL"; fi
if [ -f "$AGG_EXIT" ] && [ "$(cat "$AGG_EXIT")" != "0" ]; then overall_status="FAIL"; fi

echo "OVERALL_STATUS=$overall_status"

if [ "$overall_status" = "FAIL" ]; then
  exit 1
fi

exit 0
