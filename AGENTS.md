# Nitori v3 contributor contract

Nitori v3 is a from-scratch C++23 competitive-programming library. Its current authority is:

```text
Tutorial and public contract: /home/tnuzy/NitoriSTL/v3-Tutorial-Comprehensive.md
Semantic source:              /home/tnuzy/NitoriSTL/src-v3/
Independent tests:            /home/tnuzy/NitoriSTL/test-v3/
Deterministic benchmark:      /home/tnuzy/NitoriSTL/bench-v3/
```

V2/X 及更早实现已从活动工作树移除，只保存在
`archive/nitori-legacy-pre-v3.tar.gz` 中。压缩包不是权威：普通开发、调试、
迁移和解题不得搜索、解包或复制其内容。只有用户明确要求历史考古时，
才可在仓库外的临时目录查看。

## Design laws

1. The semantic-source budget is 131072 bytes, excluding comments and layout whitespace.
2. Use signed `nidx_t` positions and half-open `[left,right)` intervals. `nidx_t` is
   `int` by default and `long long` when every translation unit defines `NITORI_INDEX_64`.
3. Prefer expression-based templates. Do not build a concept/trait/npre registry.
4. Put mathematical, lifetime, invalidation, ownership and complexity contracts beside code.
5. Algorithms depend on the smallest callable/data port, never a mandatory backend.
6. Reuse mechanisms, not wrappers: views, functions, root algebra, graph ports and operations.
7. Do not unify objects with different semantics merely because all can be represented by integers.
8. Preserve ordered noncommutative folds and action composition order.
9. Make destructive consumption, persistent sharing and migration costs explicit.
10. Prefer short contest code, but never hide the invariant that makes it correct.

## Change workflow

1. Read the exact `src-v3` module, its local contracts and the closest `test-v3` test.
2. State the useful operation set, brute oracle, algebraic laws and failure boundary.
3. Edit every repository file only through `apply_patch`.
4. Add a fixed regression and an independent randomized/property test for subtle behavior.
5. Run a narrow three-mode gate, then the full gate only at milestones.
6. Run deterministic benchmark and semantic-size audit at structural milestones.
7. Update `v3-Tutorial-Comprehensive.md` when public behavior or contracts change.

```bash
cd /home/tnuzy/NitoriSTL
python3 test-v3/run.py TEST_STEM
python3 test-v3/run.py
python3 test-v3/audit.py
python3 bench-v3/run.py
python3 test-v3/measure.py
```

## Review gate

```text
[ ] legacy archive was not used or extracted
[ ] no unnecessary concept/trait/owner facade appeared
[ ] owner/view lifetime and root consumption are explicit
[ ] algebraic laws and action order are visible
[ ] noncommutative order is preserved where promised
[ ] complexity matches loops, allocations and historical-node growth
[ ] fixed and independent random tests attack the dangerous boundary
[ ] debug, optimized and sanitizer modes pass
[ ] deterministic checksum and source budget remain controlled
[ ] Tutorial matches the exact public implementation
```
