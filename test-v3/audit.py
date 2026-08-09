#!/usr/bin/env python3
"""Structural V3 audit: authority, independence, abstraction pressure and headers."""

from pathlib import Path
import hashlib
import os
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCE = sorted((ROOT / "src-v3").glob("*.hpp"))
TESTS = sorted((ROOT / "test-v3").glob("*.cpp"))
ARCHIVE = ROOT / "archive/nitori-legacy-pre-v3.tar.gz"
ARCHIVE_SHA256 = "95a6121575016e3b16444a391463256a50e666cd81f469b8a93b9047b4d4d913"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def code_only(text: str) -> str:
    result: list[str] = []
    i = 0
    while i < len(text):
        if text.startswith("//", i):
            end = text.find("\n", i + 2)
            i = len(text) if end < 0 else end
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            i = len(text) if end < 0 else end + 2
        elif text[i] in "\"'":
            quote = text[i]
            i += 1
            while i < len(text):
                if text[i] == "\\":
                    i += 2
                elif text[i] == quote:
                    i += 1
                    break
                else:
                    i += 1
            result.append(" ")
        else:
            result.append(text[i])
            i += 1
    return "".join(result)


require((ROOT / "v3-Tutorial-Comprehensive.md").is_file(), "missing V3 tutorial")
for legacy in ("NITORI_DOCUMENT.md", "Nitori.h", "Nitori_unsafe.h", "src", "test",
               "bench", "examples", "tools", "v2-c", "v2-nano"):
    require(not (ROOT / legacy).exists(), f"legacy path returned to active tree: {legacy}")
require(ARCHIVE.is_file(), "missing legacy archive")
require(hashlib.sha256(ARCHIVE.read_bytes()).hexdigest() == ARCHIVE_SHA256,
        "legacy archive checksum changed")
archive_allowed = {ARCHIVE, ROOT / "archive/README.md"}
require(all(path in archive_allowed for path in (ROOT / "archive").rglob("*")),
        "legacy archive was extracted or archive directory contains an unknown entry")
require(SOURCE, "src-v3 has no headers")
require(TESTS, "test-v3 has no C++ tests")

for path in SOURCE + TESTS + sorted((ROOT / "bench-v3").glob("*.cpp")):
    raw = path.read_text()
    code = code_only(raw)
    require(not re.search(r'#\s*include\s*[<"](?:Nitori(?:_unsafe)?\.h|\.\./src/)', raw),
            f"V3 includes V2 authority: {path}")
    if path in SOURCE:
        for token in ("concept", "npre", "nassert", "nresource_pool", "nnode_domain",
                      "same_domain"):
            require(not re.search(rf"\b{token}\b", code),
                    f"forbidden abstraction pressure {token}: {path}")

for path in TESTS:
    require(not re.search(r"\bassert\s*\(", code_only(path.read_text())),
            f"test disappears under NDEBUG: {path}")

for document in (ROOT / "README.md", ROOT / "v3-Tutorial-Comprehensive.md"):
    text = document.read_text()
    for target in re.findall(r"\]\((\./[^)]+)\)", text):
        require((ROOT / target).exists(), f"broken local link {target}: {document}")

compiler = os.environ.get("CXX", "g++")
flags = [compiler, "-std=c++23", "-Wall", "-Wextra", "-Wpedantic", "-Wshadow",
         "-Werror", "-fsyntax-only", "-x", "c++", "-"]
for header in SOURCE:
    subprocess.run(flags, cwd=ROOT, input=f'#include "{header.relative_to(ROOT)}"\n',
                   text=True, check=True)

subprocess.run([sys.executable, str(ROOT / "test-v3/measure.py")], check=True)
print(f"v3 structural audit passed: {len(SOURCE)} headers, {len(TESTS)} tests")
