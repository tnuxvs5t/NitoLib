#include "../src-v4/view.hpp"
#include "../src-v4/poly.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using mint = nmodint<998244353>;

int main() {
    mt19937 rng(0xf05);
    for (nidx_t round = 0; round < 3200; ++round) {
        nidx_t n = nidx_t(rng() % 90), m = nidx_t(rng() % 90);
        vector<nidx_t> left(n), right(m);
        for (auto& value : left) value = rng() % mint::mod();
        for (auto& value : right) value = rng() % mint::mod();
        auto actual = nconvolution(nall(left), span(right));
        vector<mint> expected(n && m ? n + m - 1 : 0);
        for (nidx_t i = 0; i < n; ++i)
            for (nidx_t j = 0; j < m; ++j) expected[i + j] += mint(left[i]) * mint(right[j]);
        CHECK(actual == expected);
    }

    for (nidx_t logarithm = 0; logarithm <= 15; ++logarithm) {
        vector<mint> values(nidx_t(1) << logarithm);
        for (auto& value : values) value = rng() % mint::mod();
        auto original = values;
        nntt(values);
        nntt(values, true);
        CHECK(values == original);
    }

    for (nidx_t round = 0; round < 1800; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 120);
        vector<mint> polynomial(n);
        polynomial[0] = 1 + rng() % (mint::mod() - 1);
        for (nidx_t i = 1; i < n; ++i) polynomial[i] = rng() % mint::mod();
        auto inverse = npoly_inverse(nall(polynomial), n);
        auto product = nconvolution(polynomial, inverse);
        CHECK(product[0] == mint(1));
        for (nidx_t i = 1; i < n; ++i) CHECK(product[i] == mint(0));
        CHECK(npoly_derivative(npoly_integral(polynomial)) == polynomial);
    }

    vector<mint> samples{0, 1, 8, 27, 64};
    ncomb<mint> combinations(4);
    for (nidx_t point = 0; point < 100; ++point)
        CHECK(nlagrange_consecutive(samples, mint(point), combinations) == mint(point) * point * point);
    CHECK(nlagrange_consecutive(samples, mint(998244352), combinations) == mint(-1));

    vector<mint> large_left(1 << 12), large_right(1 << 12);
    for (auto& value : large_left) value = rng() % mint::mod();
    for (auto& value : large_right) value = rng() % mint::mod();
    auto large = nconvolution(large_left, large_right);
    mint point = 17, power = 1, eval_left = 0, eval_right = 0, eval_product = 0;
    for (mint value : large_left) eval_left += value * power, power *= point;
    power = 1;
    for (mint value : large_right) eval_right += value * power, power *= point;
    power = 1;
    for (mint value : large) eval_product += value * power, power *= point;
    CHECK(eval_product == eval_left * eval_right);

    using small = nmodint<17>;
    vector<small> cycle(16);
    for (nidx_t i = 0; i < 16; ++i) cycle[i] = i;
    auto original = cycle;
    nntt<17, 3>(cycle);
    nntt<17, 3>(cycle, true);
    CHECK(cycle == original);

    cout << "v4 poly: NTT, convolution, formal inverse, calculus and interpolation passed\n";
}
