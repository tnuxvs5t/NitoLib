#pragma once
#include "math.hpp"

// coefficients[j] multiplies a[t-j-1]. The coefficient domain only needs zero,
// one, +, += and multiplication; nlinear_recurrence never divides a coefficient.
template <class A, class C, class E>
auto nlinear_recurrence(const A& initial, const C& coefficients, E index) {
    using T = remove_cvref_t<decltype(coefficients[0])>;
    nidx_t k = nlen(coefficients);
    if (!k) return T{};

    auto multiply = [&](const vector<T>& left, const vector<T>& right) {
        vector<T> product(2 * k - 1);
        for (nidx_t i = 0; i < k; ++i)
            for (nidx_t j = 0; j < k; ++j) product[i + j] += left[i] * right[j];
        for (nidx_t degree = 2 * k - 2; degree >= k; --degree)
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
    T result{};
    for (nidx_t i = 0; i < k; ++i) result += weights[i] * initial[i];
    return result;
}

// Berlekamp-Massey over an exact field. It returns c with
// a[t] = c[0]a[t-1] + ... + c[k-1]a[t-k] on the observed prefix.
template <class V>
auto nberlekamp_massey(const V& values) {
    using T = remove_cvref_t<decltype(values[0])>;
    vector<T> current{T(1)}, previous{T(1)};
    nidx_t order = 0, shift = 1;
    T last = T(1);

    for (nidx_t at = 0; at < nlen(values); ++at) {
        T discrepancy = values[at];
        for (nidx_t j = 1; j <= order; ++j)
            discrepancy += current[j] * values[at - j];
        if (discrepancy == T{}) {
            ++shift;
            continue;
        }

        auto saved = current;
        T scale = discrepancy / last;
        current.resize(max(nlen(current), nlen(previous) + shift));
        for (nidx_t j = 0; j < nlen(previous); ++j)
            current[j + shift] -= scale * previous[j];
        if (order <= at / 2) {
            order = at + 1 - order;
            previous = move(saved);
            last = discrepancy;
            shift = 1;
        } else {
            ++shift;
        }
    }

    vector<T> result(order);
    for (nidx_t j = 0; j < order; ++j) result[j] = -current[j + 1];
    return result;
}
