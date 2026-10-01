#include "../src-v4/frac.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using wide = __int128_t;
using frac = nfrac<>;

struct rational {
    wide num, den;
    rational(wide numerator, wide denominator) : num(numerator), den(denominator) {
        CHECK(denominator != 0);
        if (den < 0) num = -num, den = -den;
        wide a = num < 0 ? -num : num, b = den;
        while (b) {
            wide next = a % b;
            a = b;
            b = next;
        }
        num /= a;
        den /= a;
    }
};

template <class E, class F>
void expect_error(F&& run) {
    bool caught = false;
    try { invoke(forward<F>(run)); }
    catch (const E& error) {
        caught = true;
        CHECK(string(error.what()).starts_with("nfrac:"));
    }
    CHECK(caught);
}

template <class I, class F>
void check_result(const rational& expected, F&& run) {
    bool fits = expected.num >= numeric_limits<I>::min() &&
                expected.num <= numeric_limits<I>::max() &&
                expected.den <= numeric_limits<I>::max();
    if (!fits) return expect_error<overflow_error>(forward<F>(run));
    auto value = invoke(forward<F>(run));
    CHECK(wide(value.numerator()) == expected.num);
    CHECK(wide(value.denominator()) == expected.den);
}

template <class I>
void check_pair(nfrac<I> left, nfrac<I> right) {
    wide a = left.numerator(), ad = left.denominator();
    wide b = right.numerator(), bd = right.denominator();
    CHECK((left == right) == (a * bd == b * ad));
    CHECK((left < right) == (a * bd < b * ad));
    CHECK((left <= right) == (a * bd <= b * ad));
    CHECK((left > right) == (a * bd > b * ad));
    CHECK((left >= right) == (a * bd >= b * ad));

    for (nidx_t op = 0; op < 4; ++op) {
        auto run = [&] {
            auto value = left;
            if (op == 0) value += right;
            if (op == 1) value -= right;
            if (op == 2) value *= right;
            if (op == 3) value /= right;
            return value;
        };
        if (op == 3 && b == 0) {
            expect_error<domain_error>(run);
            continue;
        }
        wide n = 0, d = 1;
        if (op == 0) n = a * bd + b * ad, d = ad * bd;
        if (op == 1) n = a * bd - b * ad, d = ad * bd;
        if (op == 2) n = a * b, d = ad * bd;
        if (op == 3) n = a * bd, d = ad * b;
        check_result<I>(rational(n, d), run);
    }

    check_result<I>(rational(-a, ad), [&] { return -left; });
    if (a) check_result<I>(rational(ad, a), [&] { return left.inv(); });
    else expect_error<domain_error>([&] { return left.inv(); });
}

int main() {
    using small = nfrac<int8_t>;
    static_assert(frac{} == frac(0, 1));
    static_assert(frac(6, -8) == frac(-3, 4));
    static_assert(frac(1, 6) + frac(1, 3) == frac(1, 2));
    static_assert(frac(2, 3) * frac(9, 4) == frac(3, 2));
    static_assert(frac(2, 3) / frac(4, 5) == frac(5, 6));
    static_assert(frac(LLONG_MIN) - frac(LLONG_MIN) == frac{});
    static_assert(frac(LLONG_MIN) / frac(LLONG_MIN) == frac(1));
    static_assert(frac(0, LLONG_MIN).denominator() == 1);
    static_assert(is_same_v<decltype(nfrac(1, 2)), frac>);
    static_assert(!is_constructible_v<frac, double>);
    static_assert(!is_constructible_v<frac, __int128_t>);
    static_assert(is_same_v<decltype(frac{} <=> frac{}), strong_ordering>);

    CHECK(frac(ULLONG_MAX, ULLONG_MAX) == 1);
    CHECK(small(1000, 2000) == small(1, 2));
    CHECK(frac(LLONG_MAX, 2) + frac(LLONG_MAX, 2) == frac(LLONG_MAX));
    CHECK(frac(LLONG_MAX, 2) * frac(2, LLONG_MAX) == 1);
    CHECK(frac(LLONG_MIN) * frac(1, 2) == frac(LLONG_MIN / 2));
    CHECK(2 + frac(1, 2) == frac(5, 2));
    CHECK(2 - frac(1, 2) == frac(3, 2) && 2 / frac(1, 2) == 4);
    CHECK(frac(-1, 2) < 0);
    CHECK(static_cast<long double>(frac(1, 2)) == 0.5L);

    ostringstream out;
    out << frac(6, -8) << ' ' << small(1, 2) << ' ' << frac(4);
    CHECK(out.str() == "-3/4 1/2 4/1");
    expect_error<domain_error>([] { return frac(1, 0); });
    expect_error<overflow_error>([] { return frac(LLONG_MIN, -1); });
    expect_error<overflow_error>([] { return frac(1, LLONG_MIN); });
    expect_error<overflow_error>([] { return frac(ULLONG_MAX); });
    expect_error<overflow_error>([] { return small(128); });

    vector<frac> edges{frac{}, frac(1), frac(-1), frac(LLONG_MIN), frac(LLONG_MAX),
        frac(LLONG_MIN, LLONG_MAX), frac(1, LLONG_MAX), frac(-1, LLONG_MAX)};
    for (auto left : edges)
        for (auto right : edges) check_pair(left, right);
    vector<small> small_edges{0, 1, -1, -128, 127, small(-128, 127), small(1, 127)};
    for (auto left : small_edges)
        for (auto right : small_edges) check_pair(left, right);

    mt19937_64 rng(0xf12ac2026ULL);
    for (nidx_t round = 0; round < 3000; ++round) {
        long long numerator = bit_cast<long long>(rng());
        long long denominator = bit_cast<long long>(rng());
        if (!denominator) denominator = 1;
        check_result<long long>(rational(numerator, denominator),
                                [&] { return frac(numerator, denominator); });

        auto left = frac(static_cast<long long>(rng() % 21) - 10, rng() % 10 + 1);
        auto right = frac(static_cast<long long>(rng() % 21) - 10, rng() % 10 + 1);
        auto third = frac(static_cast<long long>(rng() % 21) - 10, rng() % 10 + 1);
        CHECK((left + right) + third == left + (right + third));
        CHECK((left * right) * third == left * (right * third));
        CHECK(left * (right + third) == left * right + left * third);
        if (right != 0) CHECK((left * right) / right == left);
    }

    cout << "v4 frac: exact normalization, bounded arithmetic and ordering passed\n";
}
