#pragma once
#include <bits/stdc++.h>

using namespace std;

#ifdef NITORI_INDEX_64
using nidx_t = long long;
#else
using nidx_t = int;
#endif

template <class R>
constexpr nidx_t nlen(const R& range) {
    if constexpr (requires { range.len(); }) return nidx_t(range.len());
    else return nidx_t(size(range));
}
