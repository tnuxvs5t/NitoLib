#pragma once
#include "math.hpp"

/* a[t] = sum_{j=0}^{k-1} coefficients[j]*a[t-1-j], for every t >= k.
   initial supplies at least a[0]..a[k-1]; further entries are ignored. index is a
   nonnegative integer supporting bit testing/right shift, including 128-bit types.
   Empty coefficients mean the identically zero sequence. A commutative coefficient
   ring with zero/one, multiplication and += suffices: no inverses or primality.
   Compute x^index modulo x^k - c[0]x^(k-1) - ... - c[k-1], then pair the remainder
   with the initial values. O(k^2 log(index+1)+k) ring operations, O(k) storage;
   2*k must fit nidx_t. Inputs are borrowed for this call only, never modified. */
template <class A, class C, class E>
auto nlinear_recurrence(const A& initial, const C& coefficients, E index) {
    using T = remove_cvref_t<decltype(coefficients[0])>;
    nidx_t k = nlen(coefficients);
    if (!k) return T{};
    auto multiply = [&](const vector<T>& left, const vector<T>& right) {
        vector<T> product(2 * k - 1);
        for (nidx_t i = 0; i < k; ++i)
            for (nidx_t j = 0; j < k; ++j) product[i + j] += left[i] * right[j];
        for (nidx_t degree = 2 * k - 1; degree-- > k;)
            for (nidx_t j = 0; j < k; ++j)
                product[degree - 1 - j] += product[degree] * coefficients[j];
        product.resize(k);
        return product;
    };
    vector<T> x(k), one(k);
    one[0] = T(1);
    if (k == 1) x[0] = coefficients[0];
    else x[1] = T(1);
    auto weights = npow(move(x), index, move(one), multiply);
    T answer{};
    for (nidx_t i = 0; i < k; ++i) answer += weights[i] * initial[i];
    return answer;
}

/* Berlekamp-Massey over an EXACT field (not composite-modulus rings or approximate
   floating zero tests). Return a shortest recurrence for the supplied finite prefix:
   a[t] = c[0]a[t-1] + ... + c[k-1]a[t-k], k <= t < n.
   Empty/all-zero input returns {}; trailing zero coefficients are significant and
   retained. For instance [1,0] has order 1 with coefficient 0, not order 0.
   O(n^2) field operations, O(n) storage; n+1 fits nidx_t. One nonzero discrepancy
   division per update. No statement about unseen terms follows from a finite fit.
   Prediction needs an independent recurrence guarantee; 2*K prefix terms suffice
   when a recurrence of order <= K is known to hold starting at its order. */
template <class V>
auto nberlekamp_massey(const V& values) {
    using T = remove_cvref_t<decltype(values[0])>;
    vector<T> current{T(1)}, previous{T(1)};
    nidx_t order = 0, shift = 1;
    T last = T(1);
    for (nidx_t at = 0; at < nlen(values); ++at) {
        T discrepancy = values[at];
        for (nidx_t j = 1; j <= order; ++j) discrepancy += current[j] * values[at - j];
        if (discrepancy == T{}) { ++shift; continue; }
        auto saved = current;
        T scale = discrepancy / last;
        current.resize(max(nlen(current), nlen(previous) + shift));
        for (nidx_t j = 0; j < nlen(previous); ++j) current[j + shift] -= scale * previous[j];
        if (order <= at / 2) {
            order = at + 1 - order;
            previous = move(saved);
            last = discrepancy;
            shift = 1;
        } else ++shift;
    }
    vector<T> result(order);
    for (nidx_t j = 0; j < order; ++j) result[j] = -current[j + 1];
    return result;
}
