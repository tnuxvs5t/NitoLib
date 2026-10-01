#pragma once
#include <bits/stdc++.h>

using namespace std;

#ifdef NITORI_INDEX_64
using nidx_t = long long;
#else
using nidx_t = int;
#endif
using nuidx_t = make_unsigned_t<nidx_t>;

template <class I>
inline constexpr bool nidx_wider_v =
    numeric_limits<remove_cvref_t<I>>::digits > numeric_limits<nidx_t>::digits;

template <class R>
constexpr nidx_t nlen(const R& range) {
    if constexpr (requires { range.len(); }) return nidx_t(range.len());
    else return nidx_t(size(range));
}

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
