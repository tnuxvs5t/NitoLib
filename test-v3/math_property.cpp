#include "../src-v3/math.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using mint = nmodint<1'000'000'007>;
using i128 = __int128_t;
using u128 = __uint128_t;

constexpr i128 power10(nidx_t exponent) {
    i128 value = 1;
    while (exponent--) value *= 10;
    return value;
}

int main() {
    for (long long a = -100; a <= 100; ++a)
        for (long long b = -100; b <= 100; ++b) {
            auto result = next_gcd(a, b);
            CHECK(result.gcd == gcd(a, b));
            CHECK(a * result.x + b * result.y == result.gcd);
        }

    for (nidx_t modulus = 1; modulus <= 100; ++modulus)
        for (nidx_t value = -100; value <= 100; ++value) {
            auto inverse = ninv_mod(value, modulus);
            bool exists = gcd(value, modulus) == 1;
            CHECK(bool(inverse) == exists);
            if (inverse) CHECK((value * *inverse % modulus + modulus) % modulus == 1 % modulus);
        }

    for (nidx_t modulus_a = 1; modulus_a <= 30; ++modulus_a)
        for (nidx_t modulus_b = 1; modulus_b <= 30; ++modulus_b)
            for (nidx_t a = 0; a < modulus_a; ++a)
                for (nidx_t b = 0; b < modulus_b; ++b) {
                    auto result = ncrt(a, modulus_a, b, modulus_b);
                    nidx_t brute = -1, limit = lcm(modulus_a, modulus_b);
                    for (nidx_t value = 0; value < limit; ++value)
                        if (value % modulus_a == a && value % modulus_b == b) { brute = value; break; }
                    CHECK(bool(result) == (brute >= 0));
                    if (result) CHECK(result->first == brute && result->second == limit);
                }

    constexpr i128 huge = power10(36);
    auto huge_inverse = ninv_mod(huge, 97);
    nidx_t inverse_brute = -1;
    for (nidx_t candidate = 0; candidate < 97; ++candidate)
        if (huge * candidate % 97 == 1) { inverse_brute = candidate; break; }
    CHECK(huge_inverse && *huge_inverse == inverse_brute);
    CHECK(ninv_mod(~u128(0), 97) == optional<long long>(20));
    CHECK(ninv_mod(LLONG_MIN, 97) == optional<long long>(27));

    auto huge_crt = ncrt(huge, 97, -huge, 89);
    nidx_t crt_brute = -1;
    for (nidx_t value = 0; value < 97 * 89; ++value)
        if ((i128(value) - huge) % 97 == 0 && (i128(value) + huge) % 89 == 0) {
            crt_brute = value;
            break;
        }
    CHECK(huge_crt && huge_crt->first == crt_brute && huge_crt->second == 97 * 89);
    auto widest_crt = ncrt(~u128(0), 97, ~u128(0), 89);
    CHECK(widest_crt && widest_crt->first == 4399 && widest_crt->second == 97LL * 89);
    auto extreme_crt = ncrt(LLONG_MIN, 2, LLONG_MAX, 3);
    CHECK(extreme_crt && extreme_crt->first == 4 && extreme_crt->second == 6);

    mt19937_64 rng(0xA7A);
    for (nidx_t round = 0; round < 2000; ++round) {
        i128 a = i128((u128(rng()) << 64 | rng()) >> 1);
        i128 b = i128((u128(rng()) << 64 | rng()) >> 1);
        if (rng() & 1) a = -a;
        if (rng() & 1) b = -b;
        nidx_t modulus_a = 1 + nidx_t(rng() % 40), modulus_b = 1 + nidx_t(rng() % 40);
        auto result = ncrt(a, modulus_a, b, modulus_b);
        nidx_t residue_a = nidx_t(a % modulus_a), residue_b = nidx_t(b % modulus_b);
        if (residue_a < 0) residue_a += modulus_a;
        if (residue_b < 0) residue_b += modulus_b;
        nidx_t brute = -1, limit = lcm(modulus_a, modulus_b);
        for (nidx_t value = 0; value < limit; ++value)
            if (value % modulus_a == residue_a && value % modulus_b == residue_b) {
                brute = value;
                break;
            }
        CHECK(bool(result) == (brute >= 0));
        if (result) CHECK(result->first == brute && result->second == limit);
    }

    for (nidx_t round = 0; round < 100000; ++round) {
        long long a = static_cast<long long>(rng() % 4'000'000'001ULL) - 2'000'000'000LL;
        long long b = static_cast<long long>(rng() % 4'000'000'001ULL) - 2'000'000'000LL;
        mint x = a, y = b;
        auto norm = [](long long value) {
            value %= mint::mod();
            return value < 0 ? value + mint::mod() : value;
        };
        CHECK(static_cast<long long>(x + y) == norm(norm(a) + norm(b)));
        CHECK(static_cast<long long>(x - y) == norm(norm(a) - norm(b)));
        CHECK(static_cast<long long>(x * y) ==
              static_cast<long long>(norm(a)) * norm(b) % mint::mod());
        nidx_t exponent = nidx_t(rng() % 1000);
        long long brute = 1, base = norm(a);
        for (nidx_t i = 0; i < exponent; ++i) brute = brute * base % mint::mod();
        CHECK(static_cast<long long>(x.pow(exponent)) == brute);
        if (static_cast<long long>(y))
            CHECK(static_cast<long long>(x / y * y) == static_cast<long long>(x));
    }

    ncomb<mint> combinations(300);
    vector<vector<mint>> pascal(301, vector<mint>(301));
    pascal[0][0] = 1;
    for (nidx_t n = 1; n <= 300; ++n) {
        pascal[n][0] = pascal[n][n] = 1;
        for (nidx_t k = 1; k < n; ++k) pascal[n][k] = pascal[n - 1][k - 1] + pascal[n - 1][k];
    }
    for (nidx_t n = 0; n <= 300; ++n)
        for (nidx_t k = 0; k <= n; ++k) CHECK(combinations.choose(n, k) == pascal[n][k]);

    nsieve sieve(200000);
    for (nidx_t value = 1; value <= 200000; ++value) {
        bool prime = value >= 2;
        for (nidx_t divisor = 2; 1LL * divisor * divisor <= value; ++divisor)
            if (value % divisor == 0) { prime = false; break; }
        CHECK(sieve.prime(value) == prime);
        long long rebuilt = 1;
        for (auto [factor, exponent] : sieve.factor(value))
            for (nidx_t i = 0; i < exponent; ++i) rebuilt *= factor;
        CHECK(rebuilt == value);
        nidx_t coprime = 0;
        if (value <= 1000)
            for (nidx_t x = 1; x <= value; ++x) coprime += gcd(x, value) == 1;
        if (value <= 1000) CHECK(sieve.phi(value) == coprime);
    }
}
