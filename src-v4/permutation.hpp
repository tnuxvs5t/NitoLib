#pragma once
#include "ds.hpp"

/* Three reversals rotate a writable distinct interval in O(length) swaps. */
template <class S>
constexpr void nrotate(S&& source, nidx_t first, nidx_t middle, nidx_t last) {
    auto reverse = [&](nidx_t left, nidx_t right) {
        while (left < --right) swap(source[left++], source[right]);
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

/* Lehmer digits are accumulated in mixed radix using one Fenwick availability tree. */
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

template <class Rank>
vector<nidx_t> npermutation_unrank(nidx_t n, Rank rank) {
    vector<nidx_t> digits(n), ones(n, 1);
    for (nidx_t i = n; i-- > 0;) {
        nidx_t base = n - i;
        digits[i] = static_cast<nidx_t>(rank % base);
        rank /= base;
    }

    nfenwick<nidx_t> available(ones);
    for (nidx_t i = 0; i < n; ++i) {
        nidx_t value = available.lower_bound(digits[i] + 1);
        digits[i] = value;
        available.add(value, -1);
    }
    return digits;
}

template <class N, class Rank>
requires nidx_wider_v<N>
vector<nidx_t> npermutation_unrank(N, Rank) = delete;
