#include "../src-v3/frac.hpp"
#include "../src-v3/linear.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x "\n"; abort(); } } while (false)
using oracle_integer = __int128_t;
using frac = nfrac<>;

// Independent reference normalization; bounded cross products fit signed 128 bits.
// frac_oracle.py additionally checks against Python's unbounded Fraction arithmetic.
struct rational_oracle {
    oracle_integer numerator, denominator;
    rational_oracle(oracle_integer n, oracle_integer d) : numerator(n), denominator(d) {
        CHECK(d != 0);
        if (denominator < 0) numerator = -numerator, denominator = -denominator;
        oracle_integer a = numerator < 0 ? -numerator : numerator, b = denominator;
        while (b != 0) {
            oracle_integer remainder = a % b;
            a = b;
            b = remainder;
        }
        numerator /= a;
        denominator /= a;
    }
};

template <class E, class F>
void expect_error(F operation) {
    bool caught = false;
    try { operation(); }
    catch (const E& error) {
        caught = true;
        CHECK(string(error.what()).starts_with("nfrac:"));
    }
    CHECK(caught);
}

template <class I, class F>
void check_result(const rational_oracle& expected, F operation) {
    bool fits = expected.numerator >= numeric_limits<I>::min()
        && expected.numerator <= numeric_limits<I>::max()
        && expected.denominator <= numeric_limits<I>::max();
    if (!fits) return expect_error<overflow_error>(operation);
    nfrac<I> result = operation();
    CHECK(oracle_integer(result.numerator()) == expected.numerator);
    CHECK(oracle_integer(result.denominator()) == expected.denominator);
}

template <class I>
void check_pair(nfrac<I> a, nfrac<I> b) {
    oracle_integer an = a.numerator(), ad = a.denominator();
    oracle_integer bn = b.numerator(), bd = b.denominator();
    CHECK((a == b) == (an * bd == bn * ad));
    CHECK((a != b) == (an * bd != bn * ad));
    CHECK((a < b) == (an * bd < bn * ad));
    CHECK((a <= b) == (an * bd <= bn * ad));
    CHECK((a > b) == (an * bd > bn * ad));
    CHECK((a >= b) == (an * bd >= bn * ad));
    for (nidx_t op = 0; op < 4; ++op) {
        auto operation = [&] {
            auto compound = a;
            try {
                auto binary = a;
                if (op == 0) binary = a + b, compound += b;
                if (op == 1) binary = a - b, compound -= b;
                if (op == 2) binary = a * b, compound *= b;
                if (op == 3) binary = a / b, compound /= b;
                CHECK(binary == compound);
                return compound;
            } catch (...) {
                CHECK(compound == a);
                throw;
            }
        };
        // Check the mutating path itself, including when binary arithmetic throws first.
        auto mutating = [&] {
            auto result = a;
            try {
                if (op == 0) result += b;
                if (op == 1) result -= b;
                if (op == 2) result *= b;
                if (op == 3) result /= b;
                return result;
            } catch (...) { CHECK(result == a); throw; }
        };
        if (op == 3 && bn == 0) {
            expect_error<domain_error>(operation);
            expect_error<domain_error>(mutating);
            continue;
        }
        oracle_integer n = 0, d = 1;
        if (op == 0) n = an * bd + bn * ad, d = ad * bd;
        if (op == 1) n = an * bd - bn * ad, d = ad * bd;
        if (op == 2) n = an * bn, d = ad * bd;
        if (op == 3) n = an * bd, d = ad * bn;
        rational_oracle expected(n, d);
        check_result<I>(expected, operation);
        check_result<I>(expected, mutating);
    }
    check_result<I>(rational_oracle(-an, ad), [&] { return -a; });
    if (an != 0) check_result<I>(rational_oracle(ad, an), [&] { return a.inv(); });
    else expect_error<domain_error>([&] { return a.inv(); });
}

int main() {
    static_assert(frac{} == frac(0, 1));
    static_assert(frac(6, -8) == frac(-3, 4));
    static_assert(frac(1, 6) + frac(1, 3) == frac(1, 2));
    static_assert(frac(2, 3) * frac(9, 4) == frac(3, 2));
    static_assert(frac(2, 3) / frac(4, 5) == frac(5, 6));
    static_assert(frac(1, 3) - frac(1, 2) == frac(-1, 6));
    static_assert(frac(LLONG_MIN) - frac(LLONG_MIN) == frac{});
    static_assert(frac(LLONG_MIN) / frac(LLONG_MIN) == frac(1));
    static_assert(frac(LLONG_MIN, LLONG_MIN) == frac(1));
    static_assert(frac(0, LLONG_MIN).denominator() == 1);
    static_assert(frac(-2, 3) < frac(-1, 2));
    static_assert(+frac(2) == 2 && frac(2).inv() == frac(1, 2));
    static_assert(is_same_v<decltype(nfrac(1, 2)), frac>);
    static_assert(!is_constructible_v<frac, double>);
    static_assert(!is_constructible_v<frac, __int128_t>);
    static_assert(!is_constructible_v<frac, __uint128_t>);
    static_assert(!is_constructible_v<frac, long long, double>);
    static_assert(is_same_v<decltype(frac{} <=> frac{}), strong_ordering>);
    CHECK(frac(ULLONG_MAX, ULLONG_MAX) == 1);
    CHECK(nfrac<int8_t>(1000, 2000) == nfrac<int8_t>(1, 2));
    CHECK(frac(LLONG_MAX, 2) + frac(LLONG_MAX, 2) == frac(LLONG_MAX));
    CHECK(frac(LLONG_MAX, 2) * frac(2, LLONG_MAX) == 1);
    CHECK(frac(LLONG_MIN) * frac(1, 2) == frac(LLONG_MIN / 2));
    CHECK(frac(1, LLONG_MAX) + frac(1, LLONG_MAX) == frac(2, LLONG_MAX));
    CHECK(2 + frac(1, 2) == frac(5, 2) && frac(1, 2) + 2 == frac(5, 2));
    CHECK(2 - frac(1, 2) == frac(3, 2) && 2 / frac(1, 2) == 4);
    CHECK(frac(-1, 2) < 0 && 0 > frac(-1, 2));
    CHECK(static_cast<long double>(frac(1, 2)) == 0.5L);
    CHECK(npow(frac(2, 3), 5) == frac(32, 243));
    ostringstream out;
    out << frac(6, -8) << ' ' << nfrac<int8_t>(1, 2) << ' ' << frac(4);
    CHECK(out.str() == "-3/4 1/2 4/1");

    expect_error<domain_error>([] { return frac(0, 0); });
    expect_error<domain_error>([] { return frac(1, 0); });
    expect_error<overflow_error>([] { return frac(LLONG_MIN, -1); });
    expect_error<overflow_error>([] { return frac(1, LLONG_MIN); });
    expect_error<overflow_error>([] { return frac(ULLONG_MAX); });
    expect_error<overflow_error>([] { return frac(1, ULLONG_MAX); });
    expect_error<overflow_error>([] { return nfrac<int8_t>(128); });
    expect_error<overflow_error>([] { return nfrac<int8_t>(1, 128); });

    // Minimum negation, denominator overflow, cancellation, and aliasing.
    vector<frac> edges{frac{}, frac(1), frac(-1), frac(LLONG_MIN), frac(LLONG_MAX),
        frac(LLONG_MIN, LLONG_MAX), frac(LLONG_MIN + 1, LLONG_MAX - 1),
        frac(1, LLONG_MAX), frac(-1, LLONG_MAX), frac(LLONG_MAX, LLONG_MAX - 1)};
    for (auto a : edges) {
        auto value = a;
        value -= value;
        CHECK(value == 0);
        if (a != 0) { value = a; value /= value; CHECK(value == 1); }
        for (auto b : edges) check_pair(a, b);
    }
    vector<nfrac<int8_t>> small_edges{0, 1, -1, -128, 127,
        nfrac<int8_t>(-128, 127), nfrac<int8_t>(1, 127), nfrac<int8_t>(-1, 127)};
    for (auto a : small_edges)
        for (auto b : small_edges) check_pair(a, b);
    frac alias(2, 3);
    alias += alias;
    CHECK(alias == frac(4, 3));
    alias *= alias;
    CHECK(alias == frac(16, 9));
    for (long long n : {LLONG_MIN, LLONG_MIN + 1, -2LL, -1LL, 0LL, 1LL, 2LL, LLONG_MAX})
        for (long long d : {LLONG_MIN, -2LL, -1LL, 1LL, 2LL, LLONG_MAX})
            check_result<long long>(rational_oracle(n, d), [&] { return frac(n, d); });

    for (int a = -8; a <= 8; ++a)
        for (int b = 1; b <= 8; ++b)
            for (int c = -8; c <= 8; ++c)
                for (int d = 1; d <= 8; ++d)
                    check_pair(nfrac<int8_t>(a, b), nfrac<int8_t>(c, d));

    mt19937_64 rng(0xf12ac2026ULL);
    for (nidx_t trial = 0; trial < 3000; ++trial) {
        long long a = bit_cast<long long>(rng()), b = bit_cast<long long>(rng());
        if (!b) b = 1;
        check_result<long long>(rational_oracle(a, b), [&] { return frac(a, b); });
        auto d = static_cast<long long>(rng() % LLONG_MAX) + 1;
        auto e = static_cast<long long>(rng() % LLONG_MAX) + 1;
        check_pair(frac(a, d), frac(b, e));
        uint64_t u = rng(), v = rng();
        if (!v) v = 1;
        check_result<long long>(rational_oracle(u, v), [&] { return frac(u, v); });
        // Small values keep both sides of the field laws representable.
        frac x(static_cast<long long>(rng() % 21) - 10, rng() % 10 + 1);
        frac y(static_cast<long long>(rng() % 21) - 10, rng() % 10 + 1);
        frac z(static_cast<long long>(rng() % 21) - 10, rng() % 10 + 1);
        CHECK((x + y) + z == x + (y + z));
        CHECK((x * y) * z == x * (y * z));
        CHECK(x * (y + z) == x * y + x * z);
        CHECK((x + y) - y == x);
        if (y != 0) CHECK((x * y) / y == x);
    }

    // Existing field algorithms consume nfrac without any special adapter.
    nmatrix<frac> matrix(2, 2);
    matrix.data = {2, 1, 1, -1};
    auto solution = nlinear_solve(matrix, vector<frac>{1, 0});
    CHECK(solution.consistent && solution.basis.empty());
    CHECK(solution.particular == vector<frac>({frac(1, 3), frac(1, 3)}));
    CHECK(ndeterminant(matrix) == -3);
    auto inverse = ninverse(matrix);
    CHECK(inverse.has_value());
    CHECK(nmatmul(matrix, *inverse).data == nmatrix<frac>::identity(2).data);
}
