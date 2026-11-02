#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
REPORTS_DIR="$ROOT_DIR/reports"
RUNS_DIR="$REPORTS_DIR/runs"
mkdir -p "$RUNS_DIR"

RUN_ID="run-$(date -u +%Y%m%dT%H%M%SZ)-$$"
RUN_DIR="$RUNS_DIR/$RUN_ID"
mkdir -p "$RUN_DIR"

PYTHON_LOG="$RUN_DIR/python.log"
RUST_LOG="$RUN_DIR/rust.log"
C_LOG="$RUN_DIR/c.log"

PYTHON_EXIT="$RUN_DIR/python.exit"
RUST_EXIT="$RUN_DIR/rust.exit"
C_EXIT="$RUN_DIR/c.exit"
AGG_EXIT="$RUN_DIR/aggregate.exit"

metadata_path="$RUN_DIR/metadata.json"

write_metadata() {
  cat >"$metadata_path" <<EOF
{
  "run_id": "$RUN_ID",
  "generated_at_utc": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "profile": "FEL-BU-1.0",
  "suite": "BU-KAT-001",
  "python_version": "$(python3 --version 2>/dev/null || echo 'NOT_AVAILABLE')",
  "rust_version": "$(cargo --version 2>/dev/null || echo 'NOT_AVAILABLE')",
  "c_compiler_version": "$(gcc --version 2>/dev/null | head -n 1 || echo 'NOT_AVAILABLE')",
  "commands": {
    "python": "cd \"$ROOT_DIR/python\" && python3 -m unittest -v test_berlin_uhr.py",
    "rust": "cd \"$ROOT_DIR/rust\" && cargo test -- --nocapture",
    "c": "cd \"$ROOT_DIR/c\" && gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 bu_kat_tests.c -o bu_kat_tests && ./bu_kat_tests"
  }
}
EOF
}

write_metadata

run_step() {
  local label="$1"
  local command="$2"
  local log_file="$3"
  local exit_file="$4"

  if [ "$label" = "python" ] && ! command -v python3 >/dev/null 2>&1; then
    printf '%s' "127" >"$exit_file"
    echo "[$label] UNAVAILABLE" >"$log_file"
    return
  fi

  if [ "$label" = "rust" ] && ! command -v cargo >/dev/null 2>&1; then
    printf '%s' "127" >"$exit_file"
    echo "[$label] UNAVAILABLE" >"$log_file"
    return
  fi

  if [ "$label" = "c" ] && ! command -v gcc >/dev/null 2>&1; then
    printf '%s' "127" >"$exit_file"
    echo "[$label] UNAVAILABLE" >"$log_file"
    return
  fi

  bash -lc "$command" >"$log_file" 2>&1
  local status=$?
  printf "%s" "$status" >"$exit_file"
  echo "[$label] exit_code=$status"
}

run_step "python" "cd \"$ROOT_DIR/python\" && python3 -m unittest -v test_berlin_uhr.py" "$PYTHON_LOG" "$PYTHON_EXIT"
run_step "rust" "cd \"$ROOT_DIR/rust\" && cargo test -- --nocapture" "$RUST_LOG" "$RUST_EXIT"
run_step "c" "cd \"$ROOT_DIR/c\" && gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 bu_kat_tests.c -o bu_kat_tests && ./bu_kat_tests" "$C_LOG" "$C_EXIT"

python3 "$SCRIPT_DIR/aggregate_conformance.py" --root "$ROOT_DIR" --run-dir "$RUN_DIR" --report "$RUN_DIR/conformance_report.json" --summary "$RUN_DIR/audit_summary.md"
AGG_STATUS=$?
printf "%s" "$AGG_STATUS" >"$AGG_EXIT"

if [ "$AGG_STATUS" -ne 0 ]; then
  echo "[aggregate] failed"
fi

printf "RUN_ID=%s\n" "$RUN_ID"
printf "RUN_DIR=%s\n" "$RUN_DIR"

exit "$AGG_STATUS"
