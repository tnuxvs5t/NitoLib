#pragma once
#include "core.hpp"

/* Sorted primes with multiplicity -> distinct (prime,exponent) pairs, O(n).
   Empty input represents 1. Exponents use nidx_t; prime values retain their type. */
template <class V>
auto nfactor_powers(const V& factors) {
    using I = remove_cvref_t<decltype(factors[0])>;
    vector<pair<I, nidx_t>> result;
    for (nidx_t i = 0; i < nlen(factors); ++i) {
        I prime = factors[i];
        if (result.empty() || result.back().first != prime) result.emplace_back(prime, 1);
        else ++result.back().second;
    }
    return result;
}

/* Distinct primes with positive exponents. The represented integer fits the prime
   value type; the number of divisors fits nidx_t. O(tau(n)) time/memory, unsorted.
   Both nsieve::factor and nfactor_powers(nfactor(n)) feed this operation directly. */
template <class V>
auto ndivisors(const V& factors) {
    using I = remove_cvref_t<decltype(factors[0].first)>;
    vector<I> result{I(1)};
    for (nidx_t i = 0; i < nlen(factors); ++i) {
        auto [prime, exponent] = factors[i];
        nidx_t old = nlen(result);
        I power = 1;
        for (nidx_t e = 0; e < exponent; ++e) {
            power *= prime;
            for (nidx_t j = 0; j < old; ++j) result.push_back(result[j] * power);
        }
    }
    return result;
}

/* All four transforms use indices 1..n; index 0 is untouched. Additive commutative
   group, exact +=/-=, O(n log n) time, O(1) extra storage. Views must not alias indices.
   Divisor zeta: out[x] = sum_{d|x} in[d]. Its inverse restores the original input. */
template <class V>
void ndivisor_zeta(V&& values) {
    nidx_t n = nlen(values) - 1;
    for (nidx_t divisor = n / 2; divisor > 0; --divisor)
        for (nidx_t k = 2; k <= n / divisor; ++k) values[k * divisor] += values[divisor];
}

template <class V>
void ndivisor_mobius(V&& values) {
    nidx_t n = nlen(values) - 1;
    for (nidx_t divisor = 1; divisor <= n / 2; ++divisor)
        for (nidx_t k = 2; k <= n / divisor; ++k) values[k * divisor] -= values[divisor];
}

/* Multiple zeta: out[x] = sum_{x|m, m<=n} in[m]. */
template <class V>
void nmultiple_zeta(V&& values) {
    nidx_t n = nlen(values) - 1;
    for (nidx_t divisor = 1; divisor <= n / 2; ++divisor)
        for (nidx_t k = 2; k <= n / divisor; ++k) values[divisor] += values[k * divisor];
}

template <class V>
void nmultiple_mobius(V&& values) {
    nidx_t n = nlen(values) - 1;
    for (nidx_t divisor = n / 2; divisor > 0; --divisor)
        for (nidx_t k = 2; k <= n / divisor; ++k) values[divisor] -= values[k * divisor];
}
