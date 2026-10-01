#include "../src-v3/poly.hpp"
#include "../src-v3/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x "\n"; abort(); } } while (false)
using mint = nmodint<998244353>;

template <auto Mod>
void check_lucas() {
    using M = nmodint<Mod>;
    ncomb<M> table(Mod - 1);
    vector<M> row(1, M(1));
    for (nidx_t n = 0; n <= 300; ++n) {
        for (nidx_t k = 0; k <= n; ++k) CHECK(table.lucas(n, k) == row[k]);
        CHECK(table.lucas(n, n + 1) == M(0));
        row.push_back(M(0));
        for (nidx_t k = n + 1; k > 0; --k) row[k] += row[k - 1];
    }
    __uint128_t huge = ~__uint128_t(0);
    CHECK(table.lucas(huge, huge) == M(1));
    CHECK(table.lucas(huge, __uint128_t(1)) == M(huge));
}

mint evaluate(const vector<mint>& polynomial, mint point) {
    mint answer = 0;
    for (nidx_t i = nlen(polynomial); i-- > 0;) answer = answer * point + polynomial[i];
    return answer;
}

int main() {
    check_lucas<2>();
    check_lucas<7>();
    check_lucas<17>();
    ncomb<mint> table;
    vector<mint> row(1, mint(1));
    for (nidx_t n = 0; n <= 200; ++n) {
        table.extend(n);
        table.extend(n);
        for (nidx_t k = 0; k <= n; ++k) {
            CHECK(table.choose(n, k) == row[k]);
            CHECK(nchoose_small<mint>(n, k) == row[k]);
            CHECK(nchoose_small<mint>(uint64_t(n), uint64_t(k)) == row[k]);
        }
        CHECK(nchoose_small<mint>(n, n + 1) == mint(0));
        CHECK(nchoose_small<mint>(n, nidx_t(-1)) == mint(0));
        row.push_back(mint(0));
        for (nidx_t k = n + 1; k > 0; --k) row[k] += row[k - 1];
    }
    __int128_t huge = __int128_t(1) << 100;
    CHECK(nchoose_small<mint>(huge, __int128_t(2)) == mint(huge) * mint(huge - 1) / mint(2));
    CHECK(nchoose_small<mint>(huge, huge) == mint(1));
    mt19937_64 rng(0xc0ab2026);
    for (nidx_t trial = 0; trial < 1000; ++trial) {
        nidx_t n = nidx_t(rng() % 80);
        vector<mint> values(n);
        for (mint& value : values) value = 1 + rng() % (mint::mod() - 1);
        auto inverses = ninverse_batch(span(values));
        CHECK(nlen(inverses) == n);
        for (nidx_t i = 0; i < n; ++i) CHECK(values[i] * inverses[i] == mint(1));
    }
    using composite = nmodint<15>;
    vector<composite> units{1, 2, 4, 7, 8, 11, 13, 14};
    auto inverse_units = ninverse_batch(nall(units));
    for (nidx_t i = 0; i < nlen(units); ++i) CHECK(units[i] * inverse_units[i] == composite(1));
    CHECK(npoly_integral(vector<mint>{}).size() == 1);
    CHECK(npoly_integral(vector<double>{2.0, 4.0}) == vector<double>({0.0, 2.0, 2.0}));
    // Each 1/(i+1) is representable, even when the product of all denominators is not.
    auto check_floating_integral = []<class F>() {
        vector<F> coefficients(3000, F(1));
        auto integral = npoly_integral(coefficients);
        CHECK(integral[0] == F(0));
        for (nidx_t i = 0; i < nlen(coefficients); ++i) {
            F expected = F(1) / F(i + 1);
            CHECK(isfinite(integral[i + 1]));
            CHECK(abs(integral[i + 1] - expected) <= numeric_limits<F>::epsilon() * expected);
        }
    };
    check_floating_integral.operator()<float>();
    check_floating_integral.operator()<double>();
    check_floating_integral.operator()<long double>();

    for (nidx_t trial = 0; trial < 1000; ++trial) {
        nidx_t n = 1 + nidx_t(rng() % 40);
        vector<mint> polynomial(n), samples(n);
        for (mint& coefficient : polynomial) coefficient = rng();
        for (nidx_t i = 0; i < n; ++i) samples[i] = evaluate(polynomial, mint(i));
        mint point = rng();
        CHECK(nlagrange_consecutive(nall(samples), point, table) == evaluate(polynomial, point));
        for (nidx_t i = 0; i < n; ++i)
            CHECK(nlagrange_consecutive(span(samples), mint(i), table) == samples[i]);
        CHECK(npoly_derivative(npoly_integral(polynomial)) == polynomial);
    }
    using small = nmodint<7>;
    ncomb<small> small_table(6);
    vector<small> all_points{3, 1, 4, 1, 5, 2, 6};
    for (nidx_t i = 0; i < 100; ++i)
        CHECK(nlagrange_consecutive(all_points, small(i), small_table) == all_points[i % 7]);
}
