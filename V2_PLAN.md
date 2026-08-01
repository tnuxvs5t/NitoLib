# Nitori v2 build plan

> **已完成的历史构建记录，不是 API 或使用文档。** 当前权威仅为
> `/home/tnuzy/NitoriSTL/v2/Nitori.h` 与
> `/home/tnuzy/NitoriSTL/NITORI_DOCUMENT.md`。

Nitori v2 is a clean-room redesign. The v1 implementation is not an implementation
source. Stable behavior and independently useful tests may be migrated.

## Product

- `v2/Nitori.h`: checked profile.
- `v2_unsafe/Nitori.h`: optimizer-assumption profile.
- Both headers are generated from the ordered sources in `v2_src/manifest.txt`.
- `test_v2/` is independent from `test/` and is executed against both profiles.

## Dependency direction

```text
contract/base
    -> algebra
    -> reference topology (span/view/layout)
    -> owning containers
    -> capability-constrained algorithms
    -> data structures and domain algorithms
```

Algorithms depend on capabilities, never on a default container backend.

## Stable laws

1. Indices and lengths are signed `int`; valid intervals are `[l,r)`.
2. Owning containers own storage. Views only map an integer position to a reference.
3. A generic view initially returns a real lvalue reference, not a proxy reference.
4. Preconditions have one semantic meaning:
   - checked profile diagnoses failure;
   - unsafe profile exposes the precondition to the optimizer.
5. Invalid input is never silently clamped into a different operation.
6. Representations that protect invariants are private.
7. Stateful policies are stored with `[[no_unique_address]]`.
8. Mathematical laws are declared explicitly; syntax checks cannot prove laws.
9. `v2_unsafe` is generated, never independently edited.

## Milestone gates

### M0: production bench

- deterministic amalgamation;
- no-persistent-binary test runner;
- safe/unsafe differential execution;
- sanitizer mode;
- generated-header freshness check.

### M1: reference topology and sequences

- contiguous `nspan`;
- accessor-backed `nview`;
- lvalue container adaptation, subview and stride view;
- `nvector`, non-contiguous `ndeque`, multidimensional `narray`;
- sort/reverse/search/fold/unique by minimum capability;
- matrix diagonal and deque sorting tests.

### M2: enumeration and mechanisms

- cursor enumeration independent of random access;
- scan/prefix/suffix/window;
- binary-search and monotone-search framework;
- reusable rollback log;
- scratch storage and arena.

### M3: data structures

- Fenwick with declared commutative law;
- ordered noncommutative segment aggregation;
- lazy action protocol;
- persistent and rollback structures;
- wavelet and offline-query mechanisms.

### M4: implicit structures

- explicit and implicit graph views;
- traversal engines, shortest paths and rerooting;
- matrix layouts and richer reference projections.

### M5: contest library coverage

- graph, string, integer, combinatorics, polynomial, geometry and optimization;
- migrated behavior tests plus independent brute-force/property tests;
- API manifest, examples, complexity and law audit.

## Definition of v2

The version is not v2-complete until both profiles pass the same valid-input suite,
views allocate no storage, temporary owners cannot create dangling borrowed views,
non-contiguous sequences and matrix diagonals use the ordinary sequence algorithms,
and the generated headers are reproducible from one semantic source.

## Current implementation gate

M0–M5 are implemented in the clean-room source tree. The final release gate is the
reproducibility/isolation/include audit, the full checked/unsafe valid-input suite, both
sanitizer profiles, and the published API/law/complexity manifest. Passing a narrow test
never substitutes for this final gate.

Release gate passed on 2026-08-01 with 30 ordered semantic modules and 46 independent
test programs. Generated SHA-256 digests:

- checked: `037580cd97b0413cf864902fecedd02aa4ddebd73c569d0ca6bbc222e7bba27b`
- unsafe: `a9f0454c87d9571f2a974ea436dd33d9833074ec84c93bfd4ab17f0f350ebc62`
