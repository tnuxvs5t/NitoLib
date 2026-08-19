#pragma once
#include <bits/stdc++.h>

using namespace std;

#ifdef NITORI_INDEX_64
using nidx_t = long long;
#else
using nidx_t = int;
#endif
using nuidx_t = make_unsigned_t<nidx_t>;

template <class T>
inline constexpr bool nidx_wider_v =
    integral<remove_cvref_t<T>> &&
    numeric_limits<remove_cvref_t<T>>::digits > numeric_limits<nidx_t>::digits;

/*
Nitori v3 intentionally uses expression-based templates instead of a concept/trait
registry.  A finite object used below supplies len(); an ordinary container supplied
to nall supplies size() and operator[].  Lengths and positions use nidx_t, and
valid intervals are half-open.  These are contest contracts, not runtime diagnostics.
*/
template <class A>
constexpr nidx_t nlen(const A& a) {
    if constexpr (requires { a.len(); })
        return nidx_t(a.len());
    else
        return nidx_t(size(a));
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
