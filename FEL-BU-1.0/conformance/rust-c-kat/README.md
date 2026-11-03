# FEL-BU-1.0 standalone Rust/C KAT conformance

This directory is an isolated Rust/C conformance package. It does not modify the Python reference modules, canonical `FEL-BU-1.0/vectors/bu_kat_001.json`, or the main FEL-BU report.

Both implementations consume the same tab-separated fixture at `fixtures/bu_kat_001.tsv`. The Rust test binary and C program independently compute Berlin-Uhr bits from timestamps and derive RGB/HEX using the integer CMAP formulas.

## Run

Requirements: `rustc`, a C11 compiler available as `cc` (or set `CC`), and Python 3.

From this directory:

```sh
./scripts/run_all.sh
```

The script writes separate build/run logs and exit-code files under `reports/`, and generates `reports/CONFORMANCE_REPORT.json`. The aggregate status is `PASS` only when both language runs have exit code 0; missing toolchains are recorded as `NOT_RUN` (127), not PASS. A nonzero script exit means the aggregate did not pass.

For C only, the equivalent commands are:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror c/src/main.c -o build/c_tests
./build/c_tests fixtures/bu_kat_001.tsv
```

No execution result is asserted by this source package. Run the script in the target environment and retain the generated `reports/` artifacts as execution evidence. Even a Rust/C PASS applies only to these BU/CMAP KAT checks; it is not repository-wide verification and does not cover CI, TWR, TH, or FTTHR.
