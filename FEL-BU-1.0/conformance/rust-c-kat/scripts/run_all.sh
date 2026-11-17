#!/usr/bin/env bash
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || exit 2
mkdir -p reports build
# Every run starts with clean evidence; old outputs must never be aggregated.
rm -f reports/*.log reports/*.exit reports/CONFORMANCE_REPORT.json

if ! command -v python3 >/dev/null 2>&1; then
  for lang in rust c; do
    echo "NOT_RUN: python3 unavailable; fixture validation and aggregation unavailable" > "reports/${lang}.log"
    echo 127 > "reports/${lang}.exit"
    echo "NOT_RUN: python3 unavailable" > "reports/${lang}_build.log"
    echo 127 > "reports/${lang}_build.exit"
  done
  echo "NOT_RUN: python3 unavailable; fixture not validated and report not aggregated" > reports/aggregate.log
  echo 127 > reports/aggregate.exit
  cat > reports/CONFORMANCE_REPORT.json <<'JSON'
{
  "profile": "FEL-BU-1.0 Rust/C shared KAT conformance",
  "overall_rust_c_status": "NOT_RUN",
  "rust_status": "NOT_RUN",
  "c_status": "NOT_RUN",
  "fixture_status": "NOT_RUN",
  "verified_status": "NOT_VERIFIED",
  "scope_note": "No execution verdict: Python 3 is required for fixture validation and report aggregation."
}
JSON
  exit 127
fi

python3 scripts/validate_fixture.py > reports/fixture_validation.log 2>&1
fixture_rc=$?
echo "$fixture_rc" > reports/fixture_validation.exit
if [ "$fixture_rc" -ne 0 ]; then
  for lang in rust c; do
    echo "NOT_RUN: shared fixture validation failed" > "reports/${lang}.log"
    echo 125 > "reports/${lang}.exit"
    echo "NOT_RUN: shared fixture validation failed" > "reports/${lang}_build.log"
    echo 125 > "reports/${lang}_build.exit"
  done
  python3 scripts/aggregate.py > reports/aggregate.log 2>&1
  aggregate_rc=$?
  echo "$aggregate_rc" > reports/aggregate.exit
  exit 1
fi

if ! command -v rustc >/dev/null 2>&1; then
  echo "NOT_RUN: rustc not found" > reports/rust.log
  echo 127 > reports/rust.exit
  : > reports/rust_build.log
  echo 127 > reports/rust_build.exit
else
  rustc --edition=2021 --test rust/src/lib.rs -o build/rust_tests > reports/rust_build.log 2>&1
  build_rc=$?
  echo "$build_rc" > reports/rust_build.exit
  if [ "$build_rc" -eq 0 ]; then
    ./build/rust_tests --nocapture > reports/rust.log 2>&1
    rc=$?
  else
    rc=$build_rc
    cp reports/rust_build.log reports/rust.log
  fi
  echo "$rc" > reports/rust.exit
fi

CC_BIN="${CC:-cc}"
if ! command -v "$CC_BIN" >/dev/null 2>&1; then
  echo "NOT_RUN: C compiler '$CC_BIN' not found" > reports/c.log
  echo 127 > reports/c.exit
  : > reports/c_build.log
  echo 127 > reports/c_build.exit
else
  "$CC_BIN" -std=c11 -Wall -Wextra -Wpedantic -Werror c/src/main.c -o build/c_tests > reports/c_build.log 2>&1
  build_rc=$?
  echo "$build_rc" > reports/c_build.exit
  if [ "$build_rc" -eq 0 ]; then
    ./build/c_tests fixtures/bu_kat_001.tsv > reports/c.log 2>&1
    rc=$?
  else
    rc=$build_rc
    cp reports/c_build.log reports/c.log
  fi
  echo "$rc" > reports/c.exit
fi

python3 scripts/aggregate.py > reports/aggregate.log 2>&1
aggregate_rc=$?
echo "$aggregate_rc" > reports/aggregate.exit
exit "$aggregate_rc"
