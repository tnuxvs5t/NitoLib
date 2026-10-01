#include "../src-v3/recurrence.hpp"
#include "../src-v3/linear.hpp"
#include "../src-v3/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x "\n"; abort(); } } while (false)
using mint = nmodint<998244353>;

template <class T>
vector<T> brute_sequence(vector<T> initial, const vector<T>& coefficients, nidx_t terms) {
    nidx_t k = nlen(coefficients);
    initial.resize(terms);
    for (nidx_t i = k; i < terms; ++i) {
        initial[i] = T{};
        for (nidx_t j = 0; j < k; ++j) initial[i] += coefficients[j] * initial[i - j - 1];
    }
    return initial;
}

template <class T>
bool fits(const vector<T>& sequence, const vector<T>& coefficients) {
    for (nidx_t i = nlen(coefficients); i < nlen(sequence); ++i) {
        T expected{};
        for (nidx_t j = 0; j < nlen(coefficients); ++j) expected += coefficients[j] * sequence[i - j - 1];
        if (expected != sequence[i]) return false;
    }
    return true;
}

// Search the least order by solving its coefficient equations independently of BM.
template <class T>
nidx_t least_order(const vector<T>& sequence) {
    nidx_t n = nlen(sequence);
    for (nidx_t k = 0; k <= n; ++k) {
        nmatrix<T> a(n - k, k);
        vector<T> b(n - k);
        for (nidx_t i = k; i < n; ++i) {
            b[i - k] = sequence[i];
            for (nidx_t j = 0; j < k; ++j) a(i - k, j) = sequence[i - j - 1];
        }
        if (nlinear_solve(a, b).consistent) return k;
    }
    abort();
}

struct no_division {
    uint64_t value = 0;
    no_division() = default;
    explicit no_division(uint64_t x) : value(x) {}
    no_division& operator+=(no_division other) { value += other.value; return *this; }
    friend no_division operator*(no_division a, no_division b) { return no_division(a.value * b.value); }
};

int main() {
    vector<mint> empty;
    CHECK(nberlekamp_massey(empty).empty());
    CHECK(nberlekamp_massey(vector<mint>(30)).empty());
    CHECK(nlinear_recurrence(empty, empty, ~__uint128_t(0)) == mint(0));
    CHECK(nberlekamp_massey(vector<mint>{1, 0}) == vector<mint>({0}));
    CHECK(nberlekamp_massey(vector<mint>{0, 0, 1}).size() == 3);
    vector<mint> initial{0, 1}, fibonacci{1, 1};
    CHECK(nlinear_recurrence(initial, fibonacci, 10) == mint(55));
    CHECK(nlinear_recurrence(initial, fibonacci, 0) == mint(0));
    vector<mint> zero_coefficients(4), transient{2, 3, 5, 7};
    for (nidx_t i = 0; i < 10; ++i)
        CHECK(nlinear_recurrence(transient, zero_coefficients, i) == (i < 4 ? transient[i] : mint(0)));
    __uint128_t huge = (__uint128_t(1) << 120) + 123;
    CHECK(nlinear_recurrence(vector<mint>{7}, vector<mint>{3}, huge) == mint(7) * mint(3).pow(huge));
    CHECK(nlinear_recurrence(initial, vector<mint>{2, -1}, huge) == mint(huge));
    CHECK(nlinear_recurrence(initial, vector<mint>{2, -1}, __int128_t(huge)) == mint(huge));
    CHECK(nlinear_recurrence(vector<no_division>{no_division(5)},
                             vector<no_division>{no_division(1)}, huge).value == 5);

    using binary = nmodint<2>;
    for (nidx_t length = 0; length <= 9; ++length)
        for (nidx_t mask = 0; mask < (1 << length); ++mask) {
            vector<binary> sequence(length);
            for (nidx_t i = 0; i < length; ++i) sequence[i] = (mask >> i) & 1;
            auto coefficients = nberlekamp_massey(sequence);
            CHECK(fits(sequence, coefficients));
            // Exhaustively try every binary coefficient vector of smaller order.
            for (nidx_t k = 0; k < nlen(coefficients); ++k)
                for (nidx_t pattern = 0; pattern < (1 << k); ++pattern) {
                    vector<binary> shorter(k);
                    for (nidx_t j = 0; j < k; ++j) shorter[j] = (pattern >> j) & 1;
                    CHECK(!fits(sequence, shorter));
                }
        }
    mt19937_64 rng(0xbee12026);
    for (nidx_t trial = 0; trial < 500; ++trial) {
        using small = nmodint<17>;
        vector<small> arbitrary(rng() % 20);
        for (auto& value : arbitrary) value = rng() % 17;
        auto coefficients = nberlekamp_massey(arbitrary);
        CHECK(fits(arbitrary, coefficients));
        CHECK(nlen(coefficients) == least_order(arbitrary));
    }
    for (nidx_t trial = 0; trial < 350; ++trial) {
        nidx_t k = 1 + nidx_t(rng() % 10);
        vector<mint> coefficients(k), start(k);
        for (auto& value : coefficients) value = rng() % 13;
        for (auto& value : start) value = rng() % 19;
        auto sequence = brute_sequence(start, coefficients, 90);
        auto recovered = nberlekamp_massey(span(sequence).first(2 * k));
        CHECK(nlen(recovered) <= k && fits(sequence, recovered));
        for (nidx_t i = 0; i < nlen(sequence); ++i) {
            CHECK(nlinear_recurrence(nall(start), span(coefficients), i) == sequence[i]);
            CHECK(nlinear_recurrence(sequence, recovered, i) == sequence[i]);
        }
        nmatrix<mint> companion(k, k);
        for (nidx_t j = 0; j < k; ++j) companion(0, j) = coefficients[j];
        for (nidx_t j = 1; j < k; ++j) companion(j, j - 1) = 1;
        uint64_t at = uint64_t(k) + rng() % 1000000000000ULL;
        auto power = nmatpow(companion, at - uint64_t(k - 1));
        mint expected{};
        for (nidx_t j = 0; j < k; ++j) expected += power(0, j) * start[k - j - 1];
        CHECK(nlinear_recurrence(start, coefficients, at) == expected);
    }
    using composite = nmodint<12>;
    vector<composite> a{2, 3, 4}, c{6, 8, 9};
    auto sequence = brute_sequence(a, c, 100);
    for (nidx_t i = 0; i < nlen(sequence); ++i) CHECK(nlinear_recurrence(a, c, i) == sequence[i]);
}
