#pragma once
#include "core.hpp"

// A span of unsigned integers over GF(2). The zero vector is implicit; pivot
// stores one vector per highest set bit. W is the bit width of U.
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
        return !value;
    }
    U maximize(U seed = 0) const {
        for (nidx_t bit = width; bit-- > 0;)
            seed = max(seed, U(seed ^ pivot[bit]));
        return seed;
    }
    void merge(const nxor_basis& other) {
        for (U value : other.pivot) insert(value);
    }
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

// The length must be zero or a power of two. Position zero is ordinary data.
template <class V>
void nsubset_zeta(V&& values, bool inverse = false) {
    nidx_t n = nlen(values);
    for (nidx_t bit = 1; bit < n; bit <<= 1)
        for (nidx_t mask = 0; mask < n; ++mask) if (mask & bit) {
            if (inverse) values[mask] -= values[mask ^ bit];
            else values[mask] += values[mask ^ bit];
        }
}

template <class V>
void nsuperset_zeta(V&& values, bool inverse = false) {
    nidx_t n = nlen(values);
    for (nidx_t bit = 1; bit < n; bit <<= 1)
        for (nidx_t mask = 0; mask < n; ++mask) if (!(mask & bit)) {
            if (inverse) values[mask] -= values[mask | bit];
            else values[mask] += values[mask | bit];
        }
}

// Walsh-Hadamard XOR transform. Inverse additionally requires n to be invertible
// in the coefficient ring; the function intentionally does not truncate division.
template <class V>
void nxor_transform(V&& values, bool inverse = false) {
    using T = remove_cvref_t<decltype(values[0])>;
    nidx_t n = nlen(values);
    for (nidx_t half = 1; half < n; half <<= 1)
        for (nidx_t start = 0; start < n; start += half << 1)
            for (nidx_t i = 0; i < half; ++i) {
                T left = values[start + i], right = values[start + half + i];
                values[start + i] = left + right;
                values[start + half + i] = left - right;
            }
    if (inverse && n) {
        T scale = T(1) / T(n);
        for (nidx_t i = 0; i < n; ++i) values[i] *= scale;
    }
}

enum class nbit_operation { bit_and, bit_or, bit_xor };

template <nbit_operation Operation, class X, class Y>
auto nbit_convolution(const X& left, const Y& right) {
    using T = remove_cvref_t<decltype(left[0] * right[0])>;
    nidx_t n = nlen(left);
    vector<T> a(n), b(n);
    for (nidx_t i = 0; i < n; ++i) a[i] = left[i], b[i] = right[i];

    auto transform = [&](auto& values, bool inverse) {
        if constexpr (Operation == nbit_operation::bit_and)
            nsuperset_zeta(values, inverse);
        else if constexpr (Operation == nbit_operation::bit_or)
            nsubset_zeta(values, inverse);
        else
            nxor_transform(values, inverse);
    };
    transform(a, false);
    transform(b, false);
    for (nidx_t i = 0; i < n; ++i) a[i] *= b[i];
    transform(a, true);
    return a;
}
