#!/usr/bin/env bash
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || exit 2
mkdir -p reports build

if ! command -v rustc >/dev/null 2>&1; then
  echo "NOT_RUN: rustc not found" > reports/rust.log
  echo 127 > reports/rust.exit
  : > reports/rust_build.log
  echo "NOT_RUN: rustc not found" > reports/rust_build.exit
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

python3 scripts/aggregate.py
aggregate_rc=$?
exit "$aggregate_rc"
