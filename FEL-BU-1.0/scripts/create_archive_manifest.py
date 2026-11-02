#!/usr/bin/env python3
import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


IGNORE_DIRS = {".git", "__pycache__", "target", ".venv", "venv"}
IGNORE_FILES = {".DS_Store"}


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def iter_project_files(root: Path):
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        rel = path.relative_to(root)
        parts = set(rel.parts)
        if parts & IGNORE_DIRS:
            continue
        if rel.name in IGNORE_FILES:
            continue
        if path.name.endswith(".zip"):
            continue
        yield rel, path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    root = Path(args.root).resolve()
    output = Path(args.output).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)

    manifest = {
        "profile": "FEL-BU-1.0",
        "archive_name": "FEL-BU-1.0-final.zip",
        "generated_at_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "status": "PENDING_LOCAL_EXECUTION",
        "files": [],
        "total_files": 0,
        "total_size_bytes": 0,
    }

    entries = []
    total_size = 0
    for rel, path in iter_project_files(root):
        if rel.parts[0] == "FEL-BU-1.0":
            rel = Path(*rel.parts[1:])
        size = path.stat().st_size
        total_size += size
        entries.append({
            "path": rel.as_posix(),
            "size_bytes": size,
            "sha256": sha256_file(path),
        })

    manifest["files"] = sorted(entries, key=lambda x: x["path"])
    manifest["total_files"] = len(manifest["files"])
    manifest["total_size_bytes"] = total_size

    output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
