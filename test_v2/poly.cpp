#include "common.hpp"

int main() {
    nvector<long long> a{1, 2, 3}, b{4, 5};
    ntest(nconv(a, b) == nvector<long long>({4, 13, 22, 15}));
    ntest(nconv(nvector<int>{}, nvector<int>{1}).empty());
    ntest(npoly_derivative(a) == nvector<long long>({2, 6}));
    ntest(npoly_integral(nvector<double>{2, 6}) == nvector<double>({0, 2, 3}));
    ntest(npoly_evaluate(a, 10LL) == 321);

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
}
