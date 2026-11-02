PYTHON_DIR := python
RUST_DIR := rust
C_DIR := c
SCRIPTS_DIR := scripts
ARTIFACTS_DIR := artifacts
REPORTS_DIR := reports

.PHONY: python rust c audit all clean manifest

all: python rust c

python:
	cd $(PYTHON_DIR) && python -m unittest -v test_berlin_uhr.py

rust:
	cd $(RUST_DIR) && cargo test -- --nocapture

c:
	cd $(C_DIR) && gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 bu_kat_tests.c -o bu_kat_tests && ./bu_kat_tests

audit:
	bash $(SCRIPTS_DIR)/run_fel_bu_audit.sh

manifest:
	python3 $(SCRIPTS_DIR)/create_archive_manifest.py --root . --output $(ARTIFACTS_DIR)/archive_manifest.json

clean:
	rm -f $(C_DIR)/bu_kat_tests
	rm -f $(REPORTS_DIR)/*.log $(REPORTS_DIR)/*.exit
	find . -type d -name __pycache__ -prune -exec rm -rf {} +
	find . -type d -name target -prune -exec rm -rf {} +
