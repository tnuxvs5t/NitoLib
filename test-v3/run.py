#!/usr/bin/env python3
"""Independent v3 gate: size, strict debug, optimized, and ASan/UBSan builds."""

from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TESTS = sorted((ROOT / "test-v3").glob("*.cpp"))
if len(sys.argv) > 1:
    wanted = set(sys.argv[1:])
    TESTS = [test for test in TESTS if test.stem in wanted]
    if len(TESTS) != len(wanted):
        raise SystemExit(f"unknown v3 test: {sorted(wanted - {test.stem for test in TESTS})}")
MODES = {
    "debug": ["-O0", "-g", "-D_GLIBCXX_ASSERTIONS"],
    "opt": ["-O2", "-DNDEBUG"],
    "san": ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"],
}
COMMON = ["-std=c++23", "-Wall", "-Wextra", "-Wpedantic", "-Wshadow", "-Werror"]

subprocess.run([sys.executable, str(ROOT / "test-v3/measure.py")], check=True)
with tempfile.TemporaryDirectory(prefix="nitori-v3-") as tmp:
    for mode, flags in MODES.items():
        for source in TESTS:
            binary = Path(tmp) / f"{source.stem}-{mode}"
            command = [os.environ.get("CXX", "g++"), *COMMON, *flags,
                       str(source), "-o", str(binary)]
            print("+", " ".join(command), flush=True)
            subprocess.run(command, check=True)
            subprocess.run([str(binary)], check=True)
print(f"v3 gate passed: {len(TESTS)} tests x {len(MODES)} modes")
