#pragma once
#include "core.hpp"

// Input factors are sorted and contain primes with multiplicity. Empty input is 1.
template <class V>
auto nfactor_powers(const V& factors) {
    using P = remove_cvref_t<decltype(factors[0])>;
    vector<pair<P, nidx_t>> result;
    for (nidx_t i = 0; i < nlen(factors); ++i) {
        P prime = factors[i];
        if (result.empty() || result.back().first != prime)
            result.emplace_back(prime, 1);
        else
            ++result.back().second;
    }
    return result;
}

// The input has distinct primes and positive exponents. The output is unordered.
template <class V>
auto ndivisors(const V& powers) {
    using P = remove_cvref_t<decltype(powers[0].first)>;
    vector<P> result{P(1)};
    for (nidx_t i = 0; i < nlen(powers); ++i) {
        auto [prime, exponent] = powers[i];
        nidx_t old = nlen(result);
        P power = 1;
        for (nidx_t e = 0; e < exponent; ++e) {
            power *= prime;
            for (nidx_t j = 0; j < old; ++j) result.push_back(result[j] * power);
        }
    }
    return result;
}

template <class V>
void ndivisor_zeta(V&& values) {
    nidx_t n = nlen(values);
    if (n <= 1) return;
    --n;
    for (nidx_t d = n / 2; d; --d)
        for (nidx_t k = 2; k <= n / d; ++k) values[k * d] += values[d];
}

template <class V>
void ndivisor_mobius(V&& values) {
    nidx_t n = nlen(values);
    if (n <= 1) return;
    --n;
    for (nidx_t d = 1; d <= n / 2; ++d)
        for (nidx_t k = 2; k <= n / d; ++k) values[k * d] -= values[d];
}

template <class V>
void nmultiple_zeta(V&& values) {
    nidx_t n = nlen(values);
    if (n <= 1) return;
    --n;
    for (nidx_t d = 1; d <= n / 2; ++d)
        for (nidx_t k = 2; k <= n / d; ++k) values[d] += values[k * d];
}

template <class V>
void nmultiple_mobius(V&& values) {
    nidx_t n = nlen(values);
    if (n <= 1) return;
    --n;
    for (nidx_t d = n / 2; d; --d)
        for (nidx_t k = 2; k <= n / d; ++k) values[d] -= values[k * d];
}
