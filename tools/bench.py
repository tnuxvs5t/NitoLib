#!/usr/bin/env python3
"""Compile and execute the deterministic unsafe-profile Nitori X microbench."""

from __future__ import annotations

import os
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "bench" / "bench.cpp"


def main() -> int:
    fd = os.memfd_create("nitori-x-bench", 0)
    compile_result = subprocess.run(
        [
            "g++",
            "-std=gnu++20",
            "-O2",
            "-DNDEBUG",
            "-Wall",
            "-Wextra",
            "-Werror",
            str(SOURCE),
            "-o",
            f"/proc/self/fd/{fd}",
        ],
        cwd=ROOT,
        pass_fds=(fd,),
    )
    if compile_result.returncode:
        os.close(fd)
        return compile_result.returncode
    result = subprocess.run([f"/proc/self/fd/{fd}"], cwd=ROOT, pass_fds=(fd,))
    os.close(fd)
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
