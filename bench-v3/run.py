#!/usr/bin/env python3
"""Build and run the deterministic v3 kernel baseline without repository artifacts."""

from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "bench-v3/kernel_bench.cpp"
HASH_SOURCE = ROOT / "bench-v3/hash_bench.cpp"
IO_SOURCE = ROOT / "bench-v3/io_bench.cpp"
INDEX_MODES = {
    "idx32": [],
    "idx64": ["-DNITORI_INDEX_64"],
}
with tempfile.TemporaryDirectory(prefix="nitori-v3-bench-") as tmp:
    for index_mode, index_flags in INDEX_MODES.items():
        binary = Path(tmp) / f"kernel_bench-{index_mode}"
        hash_binary = Path(tmp) / f"hash_bench-{index_mode}"
        command = [os.environ.get("CXX", "g++"), "-std=c++23", "-O2", "-DNDEBUG",
                   *index_flags, "-Wall", "-Wextra", "-Wpedantic", "-Wshadow", "-Werror",
                   str(SOURCE), "-o", str(binary)]
        print("+", " ".join(command), flush=True)
        subprocess.run(command, check=True)
        for sample in range(3):
            print(f"--- {index_mode} sample {sample + 1} ---", flush=True)
            subprocess.run([str(binary)], check=True)

        hash_command = [os.environ.get("CXX", "g++"), "-std=c++23", "-O2", "-DNDEBUG",
                        *index_flags, "-Wall", "-Wextra", "-Wpedantic", "-Wshadow", "-Werror",
                        str(HASH_SOURCE), "-o", str(hash_binary)]
        print("+", " ".join(hash_command), flush=True)
        subprocess.run(hash_command, check=True)
        for mode in ("node", "flat"):
            print(f"--- {index_mode} hash {mode} ---", flush=True)
            subprocess.run([str(hash_binary), mode], check=True)

    io_binary = Path(tmp) / "io_bench"
    io_command = [os.environ.get("CXX", "g++"), "-std=c++23", "-O2", "-DNDEBUG",
                  "-Wall", "-Wextra", "-Wpedantic", "-Wshadow", "-Werror",
                  str(IO_SOURCE), "-o", str(io_binary)]
    print("+", " ".join(io_command), flush=True)
    subprocess.run(io_command, check=True)
    for sample in range(3):
        print(f"--- io sample {sample + 1} ---", flush=True)
        subprocess.run([str(io_binary)], check=True)
