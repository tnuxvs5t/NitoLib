#pragma once
#include "math.hpp"

/* MOD is prime; length is a nonzero power of two dividing MOD-1;
   ROOT is a primitive root modulo MOD. */
template <auto MOD = 998244353, auto ROOT = 3>
void nntt(vector<nmodint<MOD>>& values, bool inverse = false) {
    using mint = nmodint<MOD>;
    nidx_t n = nidx_t(values.size());
    for (nidx_t i = 1, reversed = 0; i < n; ++i) {
        nidx_t bit = n >> 1;
        for (; reversed & bit; bit >>= 1) reversed ^= bit;
        reversed ^= bit;
        if (i < reversed) swap(values[i], values[reversed]);
    }
    for (nidx_t length = 2; length <= n; length <<= 1) {
        mint step = mint(ROOT).pow((MOD - 1) / length);
        if (inverse) step = step.inv();
        for (nidx_t start = 0; start < n; start += length) {
            mint root = 1;
            for (nidx_t i = 0; i < length / 2; ++i) {
                mint left = values[start + i];
                mint right = values[start + i + length / 2] * root;
                values[start + i] = left + right;
                values[start + i + length / 2] = left - right;
                root *= step;
            }
        }
    }
    if (inverse) {
        mint scale = mint(n).inv();
        for (mint& value : values) value *= scale;
    }
}

template <auto MOD = 998244353, auto ROOT = 3, class X, class Y>
vector<nmodint<MOD>> nconvolution(X&& left, Y&& right) {
    using mint = nmodint<MOD>;
    if (!nlen(left) || !nlen(right)) return {};
    nidx_t result_size = nlen(left) + nlen(right) - 1;
    if (1LL * nlen(left) * nlen(right) <= 256) {
        vector<mint> result(result_size);
        for (nidx_t i = 0; i < nlen(left); ++i)
            for (nidx_t j = 0; j < nlen(right); ++j) result[i + j] += mint(left[i]) * mint(right[j]);
        return result;
    }
    nidx_t size = nidx_t(bit_ceil(nuidx_t(result_size)));
    vector<mint> a(size), b(size);
    for (nidx_t i = 0; i < nlen(left); ++i) a[i] = mint(left[i]);
    for (nidx_t i = 0; i < nlen(right); ++i) b[i] = mint(right[i]);
    nntt<MOD, ROOT>(a);
    nntt<MOD, ROOT>(b);
    for (nidx_t i = 0; i < size; ++i) a[i] *= b[i];
    nntt<MOD, ROOT>(a, true);
    a.resize(result_size);
    return a;
}

template <class M>
vector<M> npoly_derivative(const vector<M>& polynomial) {
    vector<M> result(max(nidx_t(0), nidx_t(polynomial.size()) - 1));
    for (nidx_t i = 1; i < nidx_t(polynomial.size()); ++i) result[i - 1] = polynomial[i] * M(i);
    return result;
}

/* Every integer denominator 1..n is invertible. Exact coefficients use O(n)
   products and one division; intermediate products must be representable.
   Built-in floating coefficients use n independent divisions to avoid factorial
   overflow/underflow introduced solely by batching. Integral constant is zero. */
template <class M>
vector<M> npoly_integral(const vector<M>& polynomial) {
    vector<M> result(polynomial.size() + 1);
    if constexpr (is_floating_point_v<M>) {
        for (nidx_t i = 0; i < nlen(polynomial); ++i) result[i + 1] = polynomial[i] / M(i + 1);
    } else {
        vector<M> denominators(polynomial.size());
        for (nidx_t i = 0; i < nlen(polynomial); ++i) denominators[i] = M(i + 1);
        auto inverses = ninverse_batch(denominators);
        for (nidx_t i = 0; i < nlen(polynomial); ++i) result[i + 1] = polynomial[i] * inverses[i];
    }
    return result;
}

/* Samples f(0)..f(n-1) of degree < n over a field, n >= 1. Points 0..n-1
   are distinct and combinations covers n-1 with invertible factorials.
   O(n) time/memory per query; no inversion at query time. */
template <class V, class M>
M nlagrange_consecutive(const V& values, M point, const ncomb<M>& combinations) {
    nidx_t n = nlen(values);
    vector<M> prefix(n + 1, M(1));
    for (nidx_t i = 0; i < n; ++i) {
        if (point == M(i)) return M(values[i]);
        prefix[i + 1] = prefix[i] * (point - M(i));
    }
    M suffix = 1, answer = 0;
    for (nidx_t i = n; i-- > 0;) {
        M term = M(values[i]) * prefix[i] * suffix
               * combinations.inverse_factorial[i] * combinations.inverse_factorial[n - 1 - i];
        if ((n - 1 - i) & 1) answer -= term;
        else answer += term;
        suffix *= point - M(i);
    }
    return answer;
}

/* series[0] is invertible; returns the first terms coefficients of 1/series. */
template <auto MOD = 998244353, auto ROOT = 3, class V>
vector<nmodint<MOD>> npoly_inverse(V&& series, nidx_t terms) {
    using mint = nmodint<MOD>;
    if (!terms) return {};
    vector<mint> result{mint(series[0]).inv()};
    for (nidx_t size = 2; size / 2 < terms; size <<= 1) {
        nidx_t length = min(size, terms);
        vector<mint> prefix(length);
        for (nidx_t i = 0; i < min(length, nlen(series)); ++i) prefix[i] = mint(series[i]);
        auto error = nconvolution<MOD, ROOT>(prefix, result);
        error.resize(length);
        for (mint& value : error) value = -value;
        error[0] += mint(2);
        result = nconvolution<MOD, ROOT>(result, error);
        result.resize(length);
    }
    result.resize(terms);
    return result;
}
