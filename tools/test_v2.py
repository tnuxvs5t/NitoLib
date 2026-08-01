#!/usr/bin/env python3
"""Compile and run v2 tests through anonymous Linux memfd executables."""

from __future__ import annotations

import argparse
import glob
import os
from pathlib import Path
import resource
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
TEST_ROOT = ROOT / "test_v2"


def compile_to_memfd(source: Path, profile: str, sanitize: bool) -> int:
    fd = os.memfd_create(f"nitori-v2-{source.stem}", 0)
    flags = [
        "g++",
        "-std=gnu++20",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
    ]
    if sanitize:
        flags += ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"]
    else:
        flags += ["-O2"]
    if profile == "unsafe":
        flags += ["-DNITORI_TEST_UNSAFE=1", "-DNDEBUG"]
    flags += [str(source), "-o", f"/proc/self/fd/{fd}"]
    result = subprocess.run(flags, cwd=ROOT, pass_fds=(fd,))
    if result.returncode:
        os.close(fd)
        return -1000 - result.returncode
    return fd


def execute(fd: int, source: Path, quiet: bool = False) -> int:
    pid = os.fork()
    if pid == 0:
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
        if quiet:
            null = os.open(os.devnull, os.O_WRONLY)
            os.dup2(null, 1)
            os.dup2(null, 2)
        env = os.environ.copy()
        env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
        env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
        os.execve(fd, [str(source)], env)
    _, status = os.waitpid(pid, 0)
    return os.waitstatus_to_exitcode(status)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile", choices=["checked", "unsafe", "both"], default="both")
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("tests", nargs="*", help="optional test stems or filenames")
    args = parser.parse_args()

    profiles = ["checked", "unsafe"] if args.profile == "both" else [args.profile]
    sources = [Path(path) for path in sorted(glob.glob(str(TEST_ROOT / "*.cpp")))]
    if args.tests:
        selected = {name.removesuffix(".cpp") for name in args.tests}
        sources = [source for source in sources if source.stem in selected]
    if not sources:
        print("no matching v2 tests", file=sys.stderr)
        return 2

    for profile in profiles:
        for source in sources:
            death = source.stem.startswith("death_")
            if death and (profile == "unsafe" or args.sanitize):
                continue
            print(f"[{profile}{'+san' if args.sanitize else ''}] {source.name}", flush=True)
            fd = compile_to_memfd(source, profile, args.sanitize)
            if fd < 0:
                return 1
            code = execute(fd, source, quiet=death)
            os.close(fd)
            if death:
                if code == 0:
                    print(f"expected contract failure: {source}", file=sys.stderr)
                    return 1
            elif code != 0:
                print(f"failed with exit code {code}: {source}", file=sys.stderr)
                return 1
    print("v2 test bench passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
