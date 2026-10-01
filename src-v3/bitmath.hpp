#pragma once
#include "core.hpp"

/* Unsigned integer vectors over GF(2), including the top bit. This stores a span,
   not subset multiplicities: zero is always represented (the empty subset).
   insert/contains/maximize O(W); merge/ordered/kth O(W^2), memory O(W). */
template <class U = uint64_t>
struct nxor_basis {
    static_assert(numeric_limits<U>::is_integer && !numeric_limits<U>::is_signed);
    static constexpr nidx_t width = numeric_limits<U>::digits;
    array<U, width> pivot{};
    nidx_t dimension = 0;

    nidx_t rank() const { return dimension; }
    bool insert(U value) {
        for (nidx_t bit = width; bit-- > 0;) if ((value >> bit) & U(1)) {
            if (pivot[bit]) value ^= pivot[bit];
            else {
                pivot[bit] = value;
                ++dimension;
                return true;
            }
        }
        return false;
    }
    bool contains(U value) const {
        for (nidx_t bit = width; bit-- > 0;)
            if ((value >> bit) & U(1)) value ^= pivot[bit];
        return value == 0;
    }
    U maximize(U seed = 0) const {
        for (nidx_t bit = width; bit-- > 0;) seed = max(seed, U(seed ^ pivot[bit]));
        return seed;
    }
    void merge(const nxor_basis& other) {
        for (U value : other.pivot) insert(value);
    }
    /* Low-to-high reduced pivots. XORing vectors selected by binary k produces
       the zero-based k-th DISTINCT value in unsigned order, including zero. */
    vector<U> ordered() const {
        auto reduced = pivot;
        vector<U> result;
        for (nidx_t bit = 0; bit < width; ++bit) if (reduced[bit]) {
            for (nidx_t lower = 0; lower < bit; ++lower)
                if ((reduced[bit] >> lower) & U(1)) reduced[bit] ^= reduced[lower];
            result.push_back(reduced[bit]);
        }
        return result;
    }
    optional<U> kth(U k) const {
        if (dimension < width && (k >> dimension)) return nullopt;
        U result = 0;
        for (U value : ordered()) {
            if (k & U(1)) result ^= value;
            k >>= 1;
        }
        return result;
    }
};

/* Length is a nonzero power of two (empty is a no-op). Independent mutable slots,
   additive commutative group, O(n log n) time and O(1) extra storage.
   subset: out[S] = sum_{T subset S} in[T]; inverse=true undoes the transform. */
template <class V>
void nsubset_zeta(V&& values, bool inverse = false) {
    nidx_t n = nlen(values);
    for (nidx_t bit = 1; bit < n; bit <<= 1)
        for (nidx_t mask = 0; mask < n; ++mask) if (mask & bit) {
            if (inverse) values[mask] -= values[mask ^ bit];
            else values[mask] += values[mask ^ bit];
        }
}

/* superset: out[S] = sum_{T superset S} in[T]; same contract as subset zeta. */
template <class V>
void nsuperset_zeta(V&& values, bool inverse = false) {
    nidx_t n = nlen(values);
    for (nidx_t bit = 1; bit < n; bit <<= 1)
        for (nidx_t mask = 0; mask < n; ++mask) if (!(mask & bit)) {
            if (inverse) values[mask] -= values[mask | bit];
            else values[mask] += values[mask | bit];
        }
}

/* XOR Walsh-Hadamard transform, O(n log n). Same length/storage contract as zeta.
   Inverse requires n invertible in the coefficient ring; not integer truncation. */
template <class V>
void nxor_transform(V&& values, bool inverse = false) {
    using T = remove_cvref_t<decltype(values[0])>;
    nidx_t n = nlen(values);
    for (nidx_t half = 1; half < n; half <<= 1)
        for (nidx_t start = 0; start < n; start += 2 * half)
            for (nidx_t i = 0; i < half; ++i) {
                T a = values[start + i], b = values[start + half + i];
                values[start + i] = a + b;
                values[start + half + i] = a - b;
            }
    if (inverse && n) {
        T scale = T(1) / T(n);
        for (nidx_t i = 0; i < n; ++i) values[i] *= scale;
    }
}

enum class nbit_operation { bit_and, bit_or, bit_xor };

/* Equal power-of-two lengths, or both empty. Commutative coefficient ring;
   XOR additionally requires 2 invertible. O(n log n) time, O(n) extra storage.
   out[k] = sum_{i OP j = k} left[i]*right[j]. Inputs are never modified. */
template <nbit_operation Operation, class X, class Y>
auto nbit_convolution(const X& left, const Y& right) {
    using T = remove_cvref_t<decltype(left[0] * right[0])>;
    nidx_t n = nlen(left);
    vector<T> a(n), b(n);
    for (nidx_t i = 0; i < n; ++i) a[i] = left[i], b[i] = right[i];
    auto transform = [&](auto& values, bool inverse) {
        if constexpr (Operation == nbit_operation::bit_and) nsuperset_zeta(values, inverse);
        else if constexpr (Operation == nbit_operation::bit_or) nsubset_zeta(values, inverse);
        else nxor_transform(values, inverse);
    };
    transform(a, false);
    transform(b, false);
    for (nidx_t i = 0; i < n; ++i) a[i] *= b[i];
    transform(a, true);
    return a;
}
