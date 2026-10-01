#pragma once
#include "ds.hpp"

/* Rotate the writable, distinct positions [first,last) left by middle-first.
   The source yields swappable lvalues; 0 <= first <= middle <= last <= nlen(source).
   Other positions and the descriptor's domain are untouched; semantic keys remain
   stable only when their storage does not alias the rotated values.
   O(last-first) swaps and O(1) library space. */
template <class S>
constexpr void nrotate(S&& source, nidx_t first, nidx_t middle, nidx_t last) {
    auto reverse = [&](nidx_t left, nidx_t right) {
        for (--right; left < right; ++left, --right) swap(source[left], source[right]);
    };
    reverse(first, middle);
    reverse(middle, last);
    reverse(first, last);
}

template <class S, class A, class B, class C>
requires (nidx_wider_v<A> || nidx_wider_v<B> || nidx_wider_v<C>)
constexpr void nrotate(S&&, A, B, C) = delete;

template <class S>
constexpr void nrotate(S&& source, nidx_t middle) {
    nrotate(forward<S>(source), 0, middle, nlen(source));
}

template <class S, class M>
requires nidx_wider_v<M>
constexpr void nrotate(S&&, M) = delete;

/* Zero-based lexicographic rank of a permutation of [0,n). Rank supports exact
   nonnegative integer arithmetic and must represent the result. O(n log n) Fenwick
   work plus n Rank updates, O(n) positional space. The source is only borrowed. */
template <class Rank = uint64_t, class S>
Rank npermutation_rank(const S& permutation) {
    nidx_t n = nlen(permutation);
    vector<nidx_t> ones(n, 1);
    nfenwick<nidx_t> available(ones);
    Rank rank = 0;
    for (nidx_t i = 0; i < n; ++i) {
        nidx_t value = permutation[i];
        rank = rank * (n - i) + available.prefix(value);
        available.add(value, -1);
    }
    return rank;
}

/* Inverse of npermutation_rank. 0 <= rank < n! and n >= 0; Rank supports exact
   nonnegative integer division/remainder by [1,n]. O(n log n) Fenwick work plus n
   Rank divisions/remainders, O(n) positional space. */
template <class Rank>
vector<nidx_t> npermutation_unrank(nidx_t n, Rank rank) {
    vector<nidx_t> permutation(n), ones(n, 1);
    for (nidx_t i = n; i-- > 0;) {
        nidx_t base = n - i;
        permutation[i] = static_cast<nidx_t>(rank % base);
        rank /= base;
    }
    nfenwick<nidx_t> available(ones);
    for (nidx_t i = 0; i < n; ++i) {
        nidx_t value = available.lower_bound(permutation[i] + 1);
        permutation[i] = value;
        available.add(value, -1);
    }
    return permutation;
}

template <class N, class Rank>
requires nidx_wider_v<N>
vector<nidx_t> npermutation_unrank(N, Rank) = delete;
