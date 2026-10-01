#include "../src-v3/math.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x "\n"; abort(); } } while (false)
using i128 = __int128_t;
using u128 = __uint128_t;

i128 brute_floor_sum(long long n, long long modulus, long long a, long long b) {
    i128 answer = 0;
    for (long long i = 0; i < n; ++i) {
        i128 numerator = i128(a) * i + b;
        i128 quotient = numerator / modulus;
        if (numerator % modulus < 0) --quotient;
        answer += quotient;
    }
    return answer;
}

void check_sqrt(uint64_t value) {
    uint64_t root = nisqrt(value);
    CHECK(u128(root) * root <= value);
    CHECK((u128(root) + 1) * (u128(root) + 1) > value);
}

int main() {
    // The unused next Bezout coefficient can overflow int64 even when the answer fits.
    for (long long a : {LLONG_MIN, LLONG_MIN + 1, -1LL, 0LL, 1LL, LLONG_MAX})
        for (long long b : {LLONG_MIN, LLONG_MIN + 1, -1LL, 0LL, 1LL, LLONG_MAX}) {
            i128 x = a < 0 ? -i128(a) : i128(a), y = b < 0 ? -i128(b) : i128(b);
            while (y) { i128 remainder = x % y; x = y; y = remainder; }
            if (x > LLONG_MAX) continue; // gcd is not representable by negcd_result.
            auto result = next_gcd(a, b);
            CHECK(result.gcd == x);
            CHECK(i128(a) * result.x + i128(b) * result.y == x);
        }
    static_assert(nfloor_sum(4, 10, 6, 3) == 3);
    static_assert(nfloor_sum(3, 2, -1, -1) == -4);
    static_assert(nisqrt(UINT64_MAX) == UINT32_MAX);
    for (long long n = 0; n <= 16; ++n)
        for (long long modulus = 1; modulus <= 16; ++modulus)
            for (long long a = -16; a <= 16; ++a)
                for (long long b = -16; b <= 16; ++b)
                    CHECK(nfloor_sum(n, modulus, a, b) == brute_floor_sum(n, modulus, a, b));
    mt19937_64 rng(0xa1172026);
    for (nidx_t trial = 0; trial < 10000; ++trial) {
        long long n = static_cast<long long>(rng() % 50);
        long long modulus = 1 + static_cast<long long>(rng() % LLONG_MAX);
        long long a = bit_cast<long long>(rng()), b = bit_cast<long long>(rng());
        i128 x = a < 0 ? -i128(a) : i128(a), y = b < 0 ? -i128(b) : i128(b);
        while (y) { i128 remainder = x % y; x = y; y = remainder; }
        if (x <= LLONG_MAX) {
            auto result = next_gcd(a, b);
            CHECK(result.gcd == x && i128(a) * result.x + i128(b) * result.y == x);
        }
        CHECK(nfloor_sum(n, modulus, a, b) == brute_floor_sum(n, modulus, a, b));
        check_sqrt(rng());
    }
    for (long long a : {LLONG_MIN, -1LL, 0LL, 1LL, LLONG_MAX})
        for (long long b : {LLONG_MIN, -1LL, 0LL, 1LL, LLONG_MAX})
            for (long long modulus : {1LL, 2LL, LLONG_MAX})
                CHECK(nfloor_sum(17, modulus, a, b) == brute_floor_sum(17, modulus, a, b));
    CHECK(nfloor_sum(LLONG_MAX, 1, 1, 0) == i128(LLONG_MAX) * (LLONG_MAX - 1) / 2);
    CHECK(nfloor_sum(LLONG_MAX, LLONG_MAX, LLONG_MAX - 1, 0)
          == i128(LLONG_MAX - 1) * (LLONG_MAX - 2) / 2);
    for (uint64_t root : {0ULL, 1ULL, 2ULL, 65535ULL, 4294967295ULL}) {
        uint64_t square = root * root;
        check_sqrt(square);
        if (square) check_sqrt(square - 1);
        check_sqrt(square + 1);
    }
    check_sqrt(UINT64_MAX);

    for (long long n = 0; n <= 2000; ++n) {
        long long next = 1, last = -1;
        nquotient_blocks(n, [&](long long left, long long right, long long quotient) {
            CHECK(left == next && left < right && right <= n + 1);
            CHECK(last < 0 || last != quotient);
            for (long long i = left; i < right; ++i) CHECK(n / i == quotient);
            next = right;
            last = quotient;
        });
        CHECK(next == n + 1);
    }
    for (long long a = 0; a <= 100; ++a)
        for (long long b = 0; b <= 100; ++b) {
            long long next = 1;
            nquotient_blocks(a, b, [&](long long left, long long right, long long qa, long long qb) {
                CHECK(left == next && left < right && right <= min(a, b) + 1);
                for (long long i = left; i < right; ++i) CHECK(a / i == qa && b / i == qb);
                next = right;
            });
            CHECK(next == min(a, b) + 1);
        }
    long long visited = 0;
    nquotient_blocks(LLONG_MAX, 3LL, [&](long long left, long long right, long long qa, long long qb) {
        CHECK(qa == LLONG_MAX / left && qb == 3 / left);
        visited += right - left;
    });
    CHECK(visited == 3);
}
