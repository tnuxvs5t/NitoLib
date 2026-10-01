#include "../src-v4/recurrence.hpp"
#include "../src-v4/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using mint = nmodint<998244353>;

template <class T>
vector<T> build_sequence(vector<T> initial, const vector<T>& coefficients, nidx_t terms) {
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
        for (nidx_t j = 0; j < nlen(coefficients); ++j)
            expected += coefficients[j] * sequence[i - j - 1];
        if (expected != sequence[i]) return false;
    }
    return true;
}

struct no_division {
    uint64_t value = 0;
    no_division() = default;
    explicit no_division(uint64_t x) : value(x) {}
    no_division& operator+=(no_division other) { value += other.value; return *this; }
    friend no_division operator*(no_division a, no_division b) {
        return no_division(a.value * b.value);
    }
};

int main() {
    vector<mint> empty;
    CHECK(nberlekamp_massey(empty).empty());
    CHECK(nberlekamp_massey(vector<mint>(30)).empty());
    CHECK(nlinear_recurrence(empty, empty, ~__uint128_t(0)) == mint(0));
    CHECK(nberlekamp_massey(vector<mint>{1, 0}) == vector<mint>({0}));
    CHECK(nberlekamp_massey(vector<mint>{0, 0, 1}).size() == 3);

    vector<mint> start{0, 1}, fibonacci{1, 1};
    CHECK(nlinear_recurrence(start, fibonacci, 10) == mint(55));
    CHECK(nlinear_recurrence(start, fibonacci, 0) == mint(0));
    vector<mint> zero(4), transient{2, 3, 5, 7};
    for (nidx_t i = 0; i < 12; ++i)
        CHECK(nlinear_recurrence(transient, zero, i) == (i < 4 ? transient[i] : mint(0)));

    __uint128_t huge = (__uint128_t(1) << 120) + 123;
    CHECK(nlinear_recurrence(vector<mint>{7}, vector<mint>{3}, huge) ==
          mint(7) * mint(3).pow(huge));
    CHECK(nlinear_recurrence(start, vector<mint>{2, -1}, huge) == mint(huge));
    CHECK(nlinear_recurrence(vector<no_division>{no_division(5)},
                             vector<no_division>{no_division(1)}, huge).value == 5);

    using binary = nmodint<2>;
    for (nidx_t length = 0; length <= 9; ++length)
        for (nidx_t mask = 0; mask < (1 << length); ++mask) {
            vector<binary> sequence(length);
            for (nidx_t i = 0; i < length; ++i) sequence[i] = (mask >> i) & 1;
            auto coefficients = nberlekamp_massey(sequence);
            CHECK(fits(sequence, coefficients));
            for (nidx_t k = 0; k < nlen(coefficients); ++k)
                for (nidx_t pattern = 0; pattern < (1 << k); ++pattern) {
                    vector<binary> shorter(k);
                    for (nidx_t j = 0; j < k; ++j) shorter[j] = (pattern >> j) & 1;
                    CHECK(!fits(sequence, shorter));
                }
        }

    mt19937_64 rng(0xbee12026);
    for (nidx_t round = 0; round < 500; ++round) {
        using small = nmodint<17>;
        vector<small> arbitrary(rng() % 20);
        for (auto& value : arbitrary) value = rng() % 17;
        auto coefficients = nberlekamp_massey(arbitrary);
        CHECK(fits(arbitrary, coefficients));
    }
    for (nidx_t round = 0; round < 400; ++round) {
        nidx_t k = 1 + nidx_t(rng() % 10);
        vector<mint> coefficients(k), initial(k);
        for (auto& value : coefficients) value = rng() % 13;
        for (auto& value : initial) value = rng() % 19;
        auto sequence = build_sequence(initial, coefficients, 100);
        auto recovered = nberlekamp_massey(span(sequence).first(2 * k));
        CHECK(nlen(recovered) <= k && fits(sequence, recovered));
        for (nidx_t i = 0; i < nlen(sequence); ++i) {
            CHECK(nlinear_recurrence(initial, coefficients, i) == sequence[i]);
            CHECK(nlinear_recurrence(sequence, recovered, i) == sequence[i]);
        }
    }

    cout << "v4 recurrence: fast terms and exact Berlekamp-Massey passed\n";
}
