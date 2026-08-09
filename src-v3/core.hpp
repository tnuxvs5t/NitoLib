#pragma once
#include <bits/stdc++.h>

using namespace std;

/*
Nitori v3 intentionally uses expression-based templates instead of a concept/trait
registry.  A finite object used below supplies len(); an ordinary container supplied
to nall supplies size() and operator[].  Lengths and positions are signed int, and
valid intervals are half-open.  These are contest contracts, not runtime diagnostics.
*/
template <class A>
constexpr int nlen(const A& a) {
    if constexpr (requires { a.len(); })
        return int(a.len());
    else
        return int(size(a));
}
