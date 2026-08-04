#include "common.hpp"

template <class T>
concept nhas_poly_integral = requires(nvector<T> values) { npoly_integral(values); };

template <class T>
concept nhas_ntt = requires(nvector<T> values) { nconv_ntt(values, values); };

int main() {
    static_assert(!nhas_poly_integral<int> && nhas_poly_integral<double>);
    static_assert(nhas_poly_integral<nmodint<101>> && !nhas_poly_integral<nmodint<100>>);
    static_assert(nhas_ntt<nmodint<998244353>> && !nhas_ntt<nmodint<100>>);

    nvector<long long> a{1, 2, 3}, b{4, 5};
    ntest(nconv(a, b) == nvector<long long>({4, 13, 22, 15}));
    ntest(nconv(nvector<int>{}, nvector<int>{1}).empty());
    ntest(npoly_derivative(a) == nvector<long long>({2, 6}));
    ntest(npoly_integral(nvector<double>{2, 6}) == nvector<double>({0, 2, 3}));
    ntest(npoly_evaluate(a, 10LL) == 321);

    using ring = nmodint<12>;
    ntest(nconv(nvector<ring>{1, 2}, nvector<ring>{3, 4}) ==
          nvector<ring>({3, 10, 8}));

    using mint = nmodint<998244353>;
    mt19937 random(0x54f01aU);
    for (int repeat = 0; repeat < 180; ++repeat) {
        int n = 1 + random() % 180, m = 1 + random() % 180;
        nvector<mint> left(n), right(m);
        for (int i = 0; i < n; ++i)
            left[i] = random();
        for (int i = 0; i < m; ++i)
            right[i] = random();
        auto brute = nconv_naive(left, right);
        auto fast = nconv_ntt(left, right);
        ntest(fast == brute);
        auto left_view = nall(left);
        auto right_view = nall(right);
        ntest(nconv(left_view, right_view) == brute);
    }

    for (int repeat = 0; repeat < 100; ++repeat) {
        int n = 1 + random() % 180;
        nvector<mint> series(n);
        series[0] = 1 + random() % 998244352;
        for (int i = 1; i < n; ++i)
            series[i] = random();
        auto inverse = nfps_inverse(series, n);
        auto product = nconv(series, inverse);
        ntest(product[0] == mint(1));
        for (int i = 1; i < n; ++i)
            ntest(product[i] == mint{});
    }

    nvector<mint> fibonacci{0, 1};
    for (int index = 2; index < 40; ++index)
        fibonacci.push(fibonacci[index - 1] + fibonacci[index - 2]);
    auto recurrence = nberlekamp(fibonacci);
    ntest(recurrence == nvector<mint>({1, 1}));
    for (int index = 0; index < 100; ++index) {
        mint expected = index < fibonacci.len() ? fibonacci[index] : mint{};
        if (index >= fibonacci.len()) {
            mint left = 0, right = 1;
            for (int step = 0; step < index; ++step) {
                mint next = left + right;
                left = right;
                right = next;
            }
            expected = left;
        }
        ntest(nrec_nth(fibonacci, recurrence, index).val() == expected);
    }

    npoly<mint> polynomial{1, 2, 3};
    ntest(polynomial.deg() == 2 && polynomial(mint{2}) == mint{17});
    ntest(polynomial.deriv() == npoly<mint>({2, 6}));
    ntest(polynomial.integral().deriv() == polynomial);
    ntest((polynomial + npoly<mint>{-1, -2, -3}).empty());

    npoly<mint> unit{1, 2, 5, 7};
    auto owner_inverse = unit.inv(40);
    auto identity = (unit * owner_inverse).cut(40);
    ntest(identity[0] == mint{1});
    for (int index = 1; index < 40; ++index)
        ntest(identity[index] == mint{});

    npoly<mint> logarithm_source{1, 3, 4, 8, 2};
    auto logarithm = logarithm_source.log(32);
    ntest(logarithm.exp(32).cut(logarithm_source.len()) == logarithm_source);
}
