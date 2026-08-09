#!/usr/bin/env python3
"""Count semantic source bytes under src-v3; comments and layout whitespace are free."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LIMIT = 128 * 1024


def semantic_bytes(text: str) -> int:
    total = 0
    i, n = 0, len(text)
    while i < n:
        if text[i].isspace():
            i += 1
            continue
        if text.startswith("//", i):
            j = text.find("\n", i + 2)
            i = n if j < 0 else j + 1
            continue
        if text.startswith("/*", i):
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
            continue

        if text.startswith('R"', i):
            opening = text.find("(", i + 2, min(n, i + 20))
            if opening >= 0:
                delimiter = text[i + 2:opening]
                closing = text.find(")" + delimiter + '"', opening + 1)
                if closing >= 0:
                    end = closing + len(delimiter) + 2
                    total += len(text[i:end].encode())
                    i = end
                    continue

        if text[i] in "\"'":
            quote, j = text[i], i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                elif text[j] == quote:
                    j += 1
                    break
                else:
                    j += 1
            total += len(text[i:j].encode())
            i = j
            continue

        total += len(text[i].encode())
        i += 1
    return total


assert semantic_bytes("a /* free */ b // free\n") == 2
for literal in ['"a b // c"', "' '", 'R\"tag(a /* b */ // c)tag\"',
                'u8R\"_(x y)_\"']:
    assert semantic_bytes(literal) == len(literal.encode())

files = sorted(p for p in (ROOT / "src-v3").rglob("*") if p.suffix in {".hpp", ".cpp"})
counts = [(p, semantic_bytes(p.read_text())) for p in files]
for path, count in counts:
    print(f"{count:6}  {path.relative_to(ROOT)}")
discrete = next((count for path, count in counts if path.name == "discrete.hpp"), 0)
if discrete > 10 * 1024:
    raise SystemExit(f"discrete.hpp exceeded 10 KiB semantic cap: {discrete}")
used = sum(count for _, count in counts)
print(f"{used:6} / {LIMIT} semantic bytes ({used / LIMIT:.1%})")
sys.exit(used > LIMIT)
