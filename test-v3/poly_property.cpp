#include "../src-v3/poly.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using mint = nmodint<998244353>;

int main() {
    mt19937 rng(0xF05);
    for (int round = 0; round < 5000; ++round) {
        int n = int(rng() % 100), m = int(rng() % 100);
        vector<int> a(n), b(m);
        for (int& value : a) value = int(rng() % mint::mod());
        for (int& value : b) value = int(rng() % mint::mod());
        auto got = nconvolution(nall(a), nall(b));
        vector<mint> expected(n && m ? n + m - 1 : 0);
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < m; ++j) expected[i + j] += mint(a[i]) * mint(b[j]);
        CHECK(got == expected);
    }

    for (int logarithm = 0; logarithm <= 16; ++logarithm) {
        int n = 1 << logarithm;
        vector<mint> values(n), original;
        for (mint& value : values) value = rng() % mint::mod();
        original = values;
        nntt(values);
        nntt(values, true);
        CHECK(values == original);
    }

    for (int round = 0; round < 3000; ++round) {
        int n = 1 + int(rng() % 150);
        vector<mint> polynomial(n);
        polynomial[0] = 1 + rng() % (mint::mod() - 1);
        for (int i = 1; i < n; ++i) polynomial[i] = rng() % mint::mod();
        auto inverse = npoly_inverse(nall(polynomial), n);
        auto product = nconvolution(nall(polynomial), nall(inverse));
        CHECK(product[0] == mint(1));
        for (int i = 1; i < n; ++i) CHECK(product[i] == mint(0));

        auto restored = npoly_derivative(npoly_integral(polynomial));
        CHECK(restored == polynomial);
    }

    vector<mint> large_a(1 << 15), large_b(1 << 15);
    for (mint& value : large_a) value = rng() % mint::mod();
    for (mint& value : large_b) value = rng() % mint::mod();
    auto large = nconvolution(nall(large_a), nall(large_b));
    mint point = 17, power = 1, eval_a = 0, eval_b = 0, eval_result = 0;
    for (mint value : large_a) eval_a += value * power, power *= point;
    power = 1;
    for (mint value : large_b) eval_b += value * power, power *= point;
    power = 1;
    for (mint value : large) eval_result += value * power, power *= point;
    CHECK(eval_result == eval_a * eval_b);

    using small = nmodint<17>;
    vector<small> cycle(16);
    for (int i = 0; i < 16; ++i) cycle[i] = i;
    auto original = cycle;
    nntt<17, 3>(cycle);
    nntt<17, 3>(cycle, true);
    CHECK(cycle == original);
}
