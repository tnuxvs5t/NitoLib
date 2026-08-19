#include "../src-v3/math.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using i128 = __int128_t;
using u128 = __uint128_t;
using mint = nmodint<1'000'000'007>;
using wide_mint = nmodint<4'000'000'007LL>;

constexpr i128 power10(nidx_t exponent) {
    i128 value = 1;
    while (exponent--) value *= 10;
    return value;
}

int main() {
    constexpr i128 huge = power10(36);
    constexpr u128 widest = ~u128(0);
    static_assert(nmod_add(3, 4, 5) == 2);
    static_assert(nmod_sub(1, 4, 5) == 2);
    static_assert(nmod_mul(3, 4, 5) == 2);
    static_assert(nmod_neg(2, 5) == 3);
    static_assert(wide_mint::mod() == 4'000'000'007LL);
    static_assert(nmod_norm(-1, wide_mint::mod()) == 4'000'000'006LL);
    static_assert(nmod_add(4'000'000'006LL, 4'000'000'006LL, wide_mint::mod()) == 4'000'000'005LL);
    static_assert(nmod_sub(0LL, 1LL, wide_mint::mod()) == 4'000'000'006LL);
    static_assert(nmod_mul(4'000'000'006LL, 4'000'000'006LL, wide_mint::mod()) == 1);
    static_assert(nmod_neg(1LL, wide_mint::mod()) == 4'000'000'006LL);
    static_assert(static_cast<long long>(wide_mint(-1)) == 4'000'000'006LL);
    static_assert(*ninv_mod(3LL, wide_mint::mod()) == 1'333'333'336LL);
    static_assert(nmod_add(LLONG_MAX - 1, LLONG_MAX - 1, LLONG_MAX) == LLONG_MAX - 2);
    static_assert(nmod_mul(LLONG_MAX - 1, LLONG_MAX - 1, LLONG_MAX) == 1);
    static_assert(nmod_norm(huge, mint::mod()) == 2401);
    static_assert(static_cast<long long>(mint(huge)) == 2401);
    static_assert(static_cast<long long>(mint(-huge)) == 999'997'606);
    static_assert(nmod_norm(numeric_limits<i128>::min(), mint::mod()) == 360'183'865);
    static_assert(nmod_norm(widest, mint::mod()) == 279'632'276);
    static_assert(nmod_add(huge, -huge, mint::mod()) == 0);
    static_assert(nmod_sub(huge, -huge, mint::mod()) == 4802);
    static_assert(nmod_mul(huge, huge, mint::mod()) == 5'764'801);
    static_assert(nmod_neg(huge, mint::mod()) == 999'997'606);
    static_assert(nmod_add(huge, 2, mint::mod()) == 2403);
    static_assert(static_cast<long long>(mint(2).pow(huge)) == 358'860'370);

    stringstream input("1000000000000000000000000000000000000 "
                       "-1000000000000000000000000000000000000 "
                       "+1000000000000000000000000000000000000");
    mint positive, negative, explicit_positive;
    input >> positive >> negative >> explicit_positive;
    CHECK(!input.fail());
    CHECK(positive == mint(2401));
    CHECK(negative == mint(-2401));
    CHECK(explicit_positive == positive);

    wide_mint wide_left = 4'000'000'006LL;
    wide_mint wide_right = 4'000'000'005LL;
    CHECK(static_cast<long long>(wide_left + wide_right) == 4'000'000'004LL);
    CHECK(static_cast<long long>(wide_left - wide_right) == 1);
    CHECK(static_cast<long long>(wide_left * wide_right) == 2);
    CHECK(static_cast<long long>(-wide_right) == 2);

    stringstream invalid("12x");
    mint unchanged = 7;
    invalid >> unchanged;
    CHECK(invalid.fail());
    CHECK(unchanged == mint(7));

    for (nidx_t modulus = 1; modulus <= 200; ++modulus) {
        for (long long value = -1000; value <= 1000; ++value) {
            nidx_t expected = nidx_t(value % modulus);
            if (expected < 0) expected += modulus;
            CHECK(nmod_norm(value, modulus) == expected);
        }

        for (nidx_t left = 0; left < modulus; ++left)
            for (nidx_t right = 0; right < modulus; ++right) {
                CHECK(nmod_add(left, right, modulus) == (left + right) % modulus);
                CHECK(nmod_sub(left, right, modulus) == (left - right + modulus) % modulus);
                CHECK(nmod_mul(left, right, modulus) == nidx_t(1LL * left * right % modulus));
                CHECK(nmod_neg(left, modulus) == (modulus - left) % modulus);
            }
    }

    mt19937_64 rng(0x128128);
    for (nidx_t round = 0; round < 100000; ++round) {
        nidx_t modulus = nidx_t(rng() % INT_MAX) + 1;
        u128 unsigned_value = u128(rng()) << 64 | rng();
        nidx_t unsigned_expected = nidx_t(unsigned_value % unsigned(modulus));
        CHECK(nmod_norm(unsigned_value, modulus) == unsigned_expected);

        i128 signed_value = i128(unsigned_value >> 1);
        if (rng() & 1) signed_value = -signed_value;
        i128 signed_remainder = signed_value % modulus;
        if (signed_remainder < 0) signed_remainder += modulus;
        nidx_t signed_expected = nidx_t(signed_remainder);
        CHECK(nmod_norm(signed_value, modulus) == signed_expected);
        CHECK(nmod_add(unsigned_value, signed_value, modulus) ==
              nidx_t((1LL * unsigned_expected + signed_expected) % modulus));
        CHECK(nmod_sub(unsigned_value, signed_value, modulus) ==
              nidx_t((1LL * unsigned_expected - signed_expected + modulus) % modulus));
        CHECK(nmod_mul(unsigned_value, signed_value, modulus) ==
              nidx_t(1LL * unsigned_expected * signed_expected % modulus));
        CHECK(nmod_neg(signed_value, modulus) ==
              (signed_expected ? modulus - signed_expected : 0));

        i128 mint_remainder = signed_value % mint::mod();
        if (mint_remainder < 0) mint_remainder += mint::mod();
        CHECK(static_cast<long long>(mint(signed_value)) == mint_remainder);
    }
}
