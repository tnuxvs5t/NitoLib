#include "../src-v3/math.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using i128 = __int128_t;
using u128 = __uint128_t;

static_assert(ndiv_floor(5, 2) == 2);
static_assert(ndiv_floor(-5, 2) == -3);
static_assert(ndiv_floor(5, -2) == -3);
static_assert(ndiv_floor(-5, -2) == 2);
static_assert(ndiv_ceil(5, 2) == 3);
static_assert(ndiv_ceil(-5, 2) == -2);
static_assert(ndiv_ceil(5, -2) == -2);
static_assert(ndiv_ceil(-5, -2) == 3);
static_assert(ndiv_floor(5u, 2u) == 2u);
static_assert(ndiv_ceil(5u, 2u) == 3u);

constexpr i128 floor_oracle(i128 a, i128 b) {
    i128 quotient = a / b, remainder = a % b;
    if (remainder != 0 && ((remainder < 0) != (b < 0))) --quotient;
    return quotient;
}

constexpr i128 ceil_oracle(i128 a, i128 b) {
    i128 quotient = a / b, remainder = a % b;
    if (remainder != 0 && ((remainder < 0) == (b < 0))) ++quotient;
    return quotient;
}

int main() {
    for (long long a = -100; a <= 100; ++a)
        for (long long b = -100; b <= 100; ++b) if (b) {
            CHECK(i128(ndiv_floor(a, b)) == floor_oracle(a, b));
            CHECK(i128(ndiv_ceil(a, b)) == ceil_oracle(a, b));
        }

    CHECK(ndiv_floor(LLONG_MIN, 2LL) == LLONG_MIN / 2);
    CHECK(ndiv_ceil(LLONG_MIN, 2LL) == LLONG_MIN / 2);
    CHECK(ndiv_floor(LLONG_MAX, -2LL) == LLONG_MIN / 2);
    CHECK(ndiv_ceil(LLONG_MAX, -2LL) == LLONG_MIN / 2 + 1);

    mt19937_64 rng(0xD1A1DE);
    for (int round = 0; round < 20000; ++round) {
        i128 a = i128((u128(rng()) << 64 | rng()) >> 1);
        i128 b = i128(1 + rng() % 100000);
        if (rng() & 1) a = -a;
        if (rng() & 1) b = -b;
        CHECK(ndiv_floor(a, b) == floor_oracle(a, b));
        CHECK(ndiv_ceil(a, b) == ceil_oracle(a, b));

        u128 ua = (u128(rng()) << 64 | rng());
        u128 ub = 1 + rng() % 100000;
        CHECK(ndiv_floor(ua, ub) == ua / ub);
        CHECK(ndiv_ceil(ua, ub) == ua / ub + (ua % ub != 0));
    }
}
