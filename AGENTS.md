# Nitori X contributor contract

Nitori X is a GNU++20 single-header competitive-programming system. Before changing the
library, read the canonical artifacts:

```text
Public checked implementation: /home/tnuzy/NitoriSTL/Nitori.h
Canonical user document:       /home/tnuzy/NitoriSTL/NITORI_DOCUMENT.md
Semantic source order:          /home/tnuzy/NitoriSTL/src/manifest.txt
```

No skill, asset, contest directory, report or cache is an API authority. Do not create
another Nitori reference document or header snapshot.

## Product boundary

- `Nitori.h` is the checked generated header and canonical public implementation.
- `Nitori_unsafe.h` is the optimizer-assumption generated header.
- Both come from the same ordered `src/*.hpp` modules.
- `test/` is the independent valid-input, death and property test suite.
- `bench/` and `tools/` belong to the maintained Nitori X evidence bench.
- Legacy implementations, reports, build artifacts and copied headers do not belong in
  this repository.

## Design laws

1. Use signed `int` indices and lengths; intervals are `[l,r)`.
2. Owning containers own storage. Views map an integer position to a real reference.
3. Reject immediate dangling borrows from temporary owners.
4. Algorithms depend on the minimum capability, not a concrete backend or STL iterator.
5. Declare algebraic laws explicitly; syntax cannot prove laws.
6. Preserve ordered noncommutative folds and action composition order.
7. checked and unsafe have identical semantics on valid inputs.
8. Invalid input is never silently clamped into a different operation.
9. Keep representations private when they protect an invariant.
10. Prefer short contest code, but never hide the invariant, precondition or overflow
    boundary that makes it correct.

## Dependency direction

```text
contract/base → algebra → borrowed reference topology → owning storage
→ generic mechanisms → data structures/domain algorithms → I/O
```

High modules must not create reverse or cyclic dependencies. New generic dimensions must
represent real mathematical differences: value type, comparison, algebra, action,
storage backend, static capacity, persistence or rollback.

## Change workflow

1. Search `Nitori.h`, the canonical Document and the nearest test for the exact symbol.
2. Identify the public contract, mathematical invariant, failure boundary and complexity.
3. Edit semantic modules in `src/`; never edit generated headers independently.
4. Use `apply_patch` for every repository file creation, change, move or deletion.
5. Add the smallest fixed regression test and an independent brute/property test when
   the algorithm is subtle.
6. Run the narrow checked/unsafe test before any wider gate.
7. Update `NITORI_DOCUMENT.md` whenever public API, semantics, laws, complexity or recipes
   change. Do not add another reference file.

```bash
cd /home/tnuzy/NitoriSTL
python3 tools/amalgamate.py
python3 tools/amalgamate.py --check
python3 tools/test.py TEST_STEM
python3 tools/test.py
python3 tools/audit.py
```

Full tests and sanitizer audits are milestone gates, not a reflex after every local edit.
Tests compile through Linux `memfd`; do not leave build binaries in the repository.

## Review checklist

```text
[ ] exact checked-header signature searched
[ ] Document contract remains accurate
[ ] owner/view lifetime is valid
[ ] [l,r), empty range and npos behavior are explicit
[ ] integer width and sentinel arithmetic are safe
[ ] algebra/action laws are true, not merely declared
[ ] ordered operations preserve left-to-right meaning
[ ] complexity matches the implementation and total constraints
[ ] fixed and randomized tests cover the failure mode
[ ] both profiles pass the validation budget selected for this change
[ ] no duplicate authority or legacy artifact was created
```
