#!/usr/bin/env python3
"""Count semantic source bytes under src-v3; comments and layout whitespace are free."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
GENERAL_EXTENSION = 5 * 1024
LIMIT = (128 + 24 + 4 + 6) * 1024 + GENERAL_EXTENSION
MATH_BASELINE = 16484
MATH_EXTENSION = 24 * 1024
MATH_MODULES = {"math.hpp", "number.hpp", "poly.hpp", "linear.hpp", "divisor.hpp",
                "bitmath.hpp", "recurrence.hpp", "frac.hpp"}
GRAPH_BASELINE = 9396
GRAPH_EXTENSION = 4 * 1024
GRAPH_MODULES = {"graph.hpp", "graph_algo.hpp", "graph_store.hpp", "flow.hpp"}
DS_BASELINE = 62960  # Live DS modules before nreftree, 2026-09-20.
GRAPH_DS_EXTENSION = 6 * 1024
DS_MODULES = {"arena.hpp", "list.hpp", "ds.hpp", "fhq.hpp", "bag.hpp", "vec_bag.hpp",
              "segment.hpp", "wavelet.hpp", "dynamic_tree.hpp", "link_cut.hpp",
              "opt.hpp", "hash.hpp", "tree.hpp", "rooted.hpp", "reftree.hpp", "topk.hpp"}


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
math_used = sum(count for path, count in counts if path.name in MATH_MODULES)
graph_used = sum(count for path, count in counts if path.name in GRAPH_MODULES)
ds_used = sum(count for path, count in counts if path.name in DS_MODULES)
print(f"math: {math_used} bytes; extension {math_used - MATH_BASELINE} / {MATH_EXTENSION}")
if math_used > MATH_BASELINE + MATH_EXTENSION:
    raise SystemExit("math/polynomial extension exceeded its reserved 24 KiB")
print(f"graph: {graph_used} bytes; extension {graph_used - GRAPH_BASELINE} / {GRAPH_EXTENSION}")
shared_used = max(0, graph_used - GRAPH_BASELINE - GRAPH_EXTENSION) + max(0, ds_used - DS_BASELINE)
print(f"DS: {ds_used} bytes; baseline {DS_BASELINE}")
print(f"graph/DS shared extension: {shared_used} / {GRAPH_DS_EXTENSION}")
other_used = used - math_used - graph_used - ds_used
spill = max(0, shared_used - GRAPH_DS_EXTENSION)
general_used = other_used + spill
general_limit = 128 * 1024 - MATH_BASELINE - GRAPH_BASELINE - DS_BASELINE + GENERAL_EXTENSION
print(f"general: {general_used} / {general_limit} bytes (graph/DS spill {spill})")
if general_used > general_limit:
    raise SystemExit("general code and graph/DS spill exceeded the general allowance")
print(f"{used:6} / {LIMIT} semantic bytes ({used / LIMIT:.1%})")
sys.exit(used > LIMIT)
