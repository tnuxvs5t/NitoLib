#pragma once
#include "core.hpp"

/* A position plan owns its array; sorting never copies or moves source elements.
   Cost is O(n log n) comparisons/accesses and O(n) positions. Equal-key order is
   unspecified. Ordinary containers and positional descriptors both work. */
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

/* Borrowed indexed span: both arrays outlive it and keep their storage stable.
   Repeated positions alias. Construction, copying and slicing the index span
   allocate nothing. Cost per access is one index lookup plus one value lookup. */
template <class T>
struct nindexed_span {
    span<T> values;
    span<const nidx_t> positions;
    nidx_t size() const { return nidx_t(positions.size()); }
    T& operator[](nidx_t i) const { return values[positions[i]]; }
};

template <class T, size_t N, class I, size_t M>
requires same_as<remove_const_t<I>, nidx_t>
auto ngather(span<T, N> values, span<I, M> positions) {
    return nindexed_span<T>{values, positions};
}

/* Explicit snapshot of maximal adjacent-equal intervals. No descriptor or owner
   survives the call. O(n) comparisons and O(number of runs) output storage. */
template <class S, class Equal = equal_to<>>
vector<pair<nidx_t, nidx_t>> nrun_bounds(S&& values, Equal equal = {}) {
    vector<pair<nidx_t, nidx_t>> bounds;
    nidx_t n = nlen(values);
    for (nidx_t left = 0; left < n;) {
        nidx_t right = left + 1;
        while (right < n && invoke(equal, values[right - 1], values[right])) ++right;
        bounds.emplace_back(left, right);
        left = right;
    }
    return bounds;
}
