#include "../src-v4/math.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using mint = nmodint<1'000'000'007>;
using i128 = __int128_t;
using u128 = __uint128_t;

int main() {
    for (long long a = -100; a <= 100; ++a)
        for (long long b = -20; b <= 20; ++b) if (b) {
            long double quotient = static_cast<long double>(a) / b;
            CHECK(ndiv_floor(a, b) == static_cast<long long>(floor(quotient)));
            CHECK(ndiv_ceil(a, b) == static_cast<long long>(ceil(quotient)));
        }
    for (long long n = 0; n <= 80; ++n)
        for (long long mod = 1; mod <= 17; ++mod)
            for (long long a = -20; a <= 20; ++a)
                for (long long b = -20; b <= 20; ++b) {
                    __int128_t expected = 0;
                    for (long long i = 0; i < n; ++i)
                        expected += ndiv_floor(a * i + b, mod);
                    CHECK(nfloor_sum(n, mod, a, b) == expected);
                }
    for (uint64_t x : {uint64_t(0), uint64_t(1), uint64_t(2),
                       uint64_t(3), uint64_t(4), uint64_t(1) << 63,
                       numeric_limits<uint64_t>::max()}) {
        uint64_t root = nisqrt(x);
        CHECK(root * root <= x &&
              (__uint128_t(root) + 1) * (__uint128_t(root) + 1) > x);
    }
    {
        vector<array<long long, 3>> blocks;
        nquotient_blocks(37, [&](long long left, long long right, long long q) {
            blocks.push_back({left, right, q});
        });
        for (auto [left, right, q] : blocks)
            for (long long i = left; i < right; ++i) CHECK(37 / i == q);
        CHECK(blocks.front()[0] == 1 && blocks.back()[1] == 38);
    }
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

    for (nidx_t a = 1; a <= 20; ++a)
        for (nidx_t b = 1; b <= 20; ++b)
            for (nidx_t x = 0; x < a; ++x)
                for (nidx_t y = 0; y < b; ++y) {
                    auto result = ncrt(x, a, y, b);
                    nidx_t limit = lcm(a, b), answer = -1;
                    for (nidx_t z = 0; z < limit; ++z)
                        if (z % a == x && z % b == y) { answer = z; break; }
                    CHECK(bool(result) == (answer >= 0));
                    if (result) CHECK(result->first == answer && result->second == limit);
                }

    mt19937_64 rng(0xA7A);
    for (nidx_t round = 0; round < 30000; ++round) {
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
    CHECK(nchoose_small<mint>(100, 3) == combinations.choose(100, 3));
    vector<mint> invertible{2, 3, 5, 7};
    auto inverses = ninverse_batch(invertible);
    for (nidx_t i = 0; i < nidx_t(invertible.size()); ++i)
        CHECK(invertible[i] * inverses[i] == mint(1));

    nsieve sieve(100000);
    for (nidx_t value = 1; value <= 100000; ++value) {
        bool prime = value >= 2;
        for (nidx_t d = 2; d * d <= value; ++d)
            if (value % d == 0) { prime = false; break; }
        CHECK(sieve.prime(value) == prime);
        nidx_t rebuilt = 1;
        for (auto [factor, exponent] : sieve.factor(value))
            for (nidx_t i = 0; i < exponent; ++i) rebuilt *= factor;
        CHECK(rebuilt == value);
    }
    cout << "v4 math: division, CRT, modular arithmetic, combinations and sieve passed\n";
}
