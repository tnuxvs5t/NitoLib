#!/usr/bin/env python3
"""Run the reproducibility, compile, runtime and sanitizer gates for Nitori X."""

from __future__ import annotations

from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]


def run(*command: str, input_text: str | None = None) -> None:
    print("+", " ".join(command), flush=True)
    result = subprocess.run(command, cwd=ROOT, input=input_text, text=True)
    if result.returncode:
        raise SystemExit(result.returncode)


def isolation_gate() -> None:
    forbidden = ["Nitori_naive", "NITORI_V2_CHECKED", "NITORI_V2_UNSAFE"]
    files = list((ROOT / "src").glob("*.hpp")) + list((ROOT / "test").glob("*"))
    for path in files:
        if not path.is_file():
            continue
        text = path.read_text()
        for needle in forbidden:
            if needle in text:
                print(f"legacy isolation failure: {needle!r} in {path}", file=sys.stderr)
                raise SystemExit(1)


def include_gate(header: str, macro: str) -> None:
    source = f'#include "{header}"\nstatic_assert({macro});\nint main(){{return nversion!=30000;}}\n'
    run("g++", "-std=gnu++20", "-O2", "-Wall", "-Wextra", "-Werror", "-fsyntax-only", "-x", "c++", "-", input_text=source)


def main() -> int:
    isolation_gate()
    run("python3", "tools/amalgamate.py", "--check")
    include_gate("Nitori.h", "!nunsafe")
    include_gate("Nitori_unsafe.h", "nunsafe")
    run("python3", "tools/test.py")
    run("python3", "tools/test.py", "--profile", "checked", "--sanitize")
    run("python3", "tools/test.py", "--profile", "unsafe", "--sanitize")
    print("Nitori X audit passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
