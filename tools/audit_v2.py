#!/usr/bin/env python3
"""Run the reproducibility, isolation, compile, runtime and sanitizer gates for v2."""

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
    forbidden = [
        '#include "../Nitori.h"',
        '#include "Nitori.h"',
        "Nitori_naive",
    ]
    files = list((ROOT / "v2_src").glob("*.hpp")) + list((ROOT / "test_v2").glob("*"))
    for path in files:
        if not path.is_file():
            continue
        text = path.read_text()
        for needle in forbidden:
            if needle in text:
                print(f"v1 isolation failure: {needle!r} in {path}", file=sys.stderr)
                raise SystemExit(1)


def include_gate(header: str, macro: str) -> None:
    source = f'#include "{header}"\nstatic_assert({macro});\nint main(){{return nversion!=20000;}}\n'
    run("g++", "-std=gnu++20", "-O2", "-Wall", "-Wextra", "-Werror", "-fsyntax-only", "-x", "c++", "-", input_text=source)


def main() -> int:
    isolation_gate()
    run("python3", "tools/amalgamate_v2.py", "--check")
    include_gate("v2/Nitori.h", "!nunsafe")
    include_gate("v2_unsafe/Nitori.h", "nunsafe")
    run("python3", "tools/test_v2.py")
    run("python3", "tools/test_v2.py", "--profile", "checked", "--sanitize")
    run("python3", "tools/test_v2.py", "--profile", "unsafe", "--sanitize")
    print("Nitori v2 audit passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
