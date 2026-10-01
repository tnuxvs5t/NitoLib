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

1. The semantic-source budget is 171008 bytes (128 + 5 + 24 + 4 + 6 KiB), excluding comments and
   layout whitespace. The added 5 KiB belongs to the general allowance, not a dedicated module.
   The additional 24 KiB is reserved for math/polynomial development;
   test-v3/measure.py records the 16484-byte math baseline and checks its extension cap.
   A separate 4 KiB is reserved for graph development above its 9396-byte baseline.
   A further 6 KiB is shared by graph and DS development: charge graph above 13492
   bytes plus DS above its 62960-byte baseline (2026-09-20). Excess graph/DS work is
   charged to the general allowance, not hidden in a separate module. DS module
   membership is explicit in test-v3/measure.py; unrelated modules cannot spend the
   reserved graph/DS allowance.
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
11. Prefer readable recursive lambda DFS for the user's CF/ICPC workflow. Do not
    replace it solely to satisfy a default local stack limit; document actual stack
    use and configure validation accordingly. Iteration needs a separate concrete benefit.

## Change workflow

**User directive (2026-09-20): full-suite testing is prohibited.** Run only the
smallest set of tests directly relevant to the change. Do not run `test-v3/run.py`
without explicit test stems, enumerate all stems, or reconstruct the full suite
through shards, parallel jobs or repeated batches. Milestones, cleanup and generic
skill instructions do not override this rule. Do not launch the whole-library
header-compilation audit or the full benchmark runner as a substitute; inspect or
compile only affected headers and run only relevant benchmark workloads when needed.
Documentation-only cleanup needs no C++ tests. Stop after relevant checks pass;
repeat or expand only for a concrete new change, failure or unresolved concern.

1. Read the exact `src-v3` module, its local contracts and the closest `test-v3` test.
2. State the useful operation set, brute oracle, algebraic laws and failure boundary.
3. Edit every repository file only through `apply_patch`.
4. Add a fixed regression and an independent randomized/property test for subtle behavior.
5. Run the smallest relevant gate; select explicit test stems. Never run the full suite.
6. Run an affected deterministic benchmark only when justified; use the lightweight
   semantic-size audit for source/budget changes.
7. Update `v3-Tutorial-Comprehensive.md` when public behavior or contracts change.

```bash
cd /home/tnuzy/NitoriSTL
python3 test-v3/run.py TEST_STEM
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
[ ] selected relevant tests pass in the required build/index modes; no full suite ran
[ ] relevant benchmark evidence (when needed) and source budget remain controlled
[ ] Tutorial matches the exact public implementation
```
