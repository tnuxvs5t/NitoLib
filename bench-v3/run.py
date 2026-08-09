#!/usr/bin/env python3
"""Build and run the deterministic v3 kernel baseline without repository artifacts."""

from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "bench-v3/kernel_bench.cpp"
with tempfile.TemporaryDirectory(prefix="nitori-v3-bench-") as tmp:
    binary = Path(tmp) / "kernel_bench"
    command = [os.environ.get("CXX", "g++"), "-std=c++23", "-O2", "-DNDEBUG",
               "-Wall", "-Wextra", "-Wpedantic", "-Wshadow", "-Werror",
               str(SOURCE), "-o", str(binary)]
    print("+", " ".join(command), flush=True)
    subprocess.run(command, check=True)
    for sample in range(3):
        print(f"--- sample {sample + 1} ---", flush=True)
        subprocess.run([str(binary)], check=True)
