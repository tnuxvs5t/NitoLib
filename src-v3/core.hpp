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

/*
Conditional scalar updates use the ordinary strict order.  They return whether the
target changed; the candidate must be comparable with and assignable to the target
when the condition is true.  An rvalue candidate may be moved into the target.
*/
template <class A, class B>
constexpr bool nchmin(A& target, B&& candidate) {
    if (candidate < target) {
        target = forward<B>(candidate);
        return true;
    }
    return false;
}

template <class A, class B>
constexpr bool nchmax(A& target, B&& candidate) {
    if (target < candidate) {
        target = forward<B>(candidate);
        return true;
    }
    return false;
}
