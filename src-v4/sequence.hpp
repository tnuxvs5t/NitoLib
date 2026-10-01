#pragma once
#include "core.hpp"

/* A position plan owns only indices; it never moves source values. */
template <class S, class C = less<>, class P = identity>
vector<nidx_t> nargsort(const S& source, C compare = {}, P projection = {}) {
    vector<nidx_t> order(nlen(source));
    iota(order.begin(), order.end(), 0);
    ranges::sort(order, [&](nidx_t left, nidx_t right) {
        return invoke(compare, invoke(projection, source[left]),
                       invoke(projection, source[right]));
    });
    return order;
}

template <class T>
struct nindexed_span {
    span<T> values;
    span<const nidx_t> positions;

    nidx_t len() const { return nidx_t(positions.size()); }
    nidx_t size() const { return len(); }
    T& operator[](nidx_t i) const { return values[positions[i]]; }
};

template <class T, size_t N, class I, size_t M>
requires same_as<remove_const_t<I>, nidx_t>
auto ngather(span<T, N> values, span<I, M> positions) {
    return nindexed_span<T>{values, positions};
}

/* Snapshot maximal adjacent-equal intervals; no descriptor escapes this call. */
template <class S, class E = equal_to<>>
vector<pair<nidx_t, nidx_t>> nrun_bounds(S&& source, E equal = {}) {
    vector<pair<nidx_t, nidx_t>> bounds;
    nidx_t n = nlen(source);
    for (nidx_t left = 0; left < n;) {
        nidx_t right = left + 1;
        while (right < n && invoke(equal, source[right - 1], source[right])) ++right;
        bounds.emplace_back(left, right);
        left = right;
    }
    return bounds;
}
