#include "../src-v3/math.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using mint = nmodint<1'000'000'007>;

int main() {
    for (long long a = -100; a <= 100; ++a)
        for (long long b = -100; b <= 100; ++b) {
            auto result = next_gcd(a, b);
            CHECK(result.gcd == gcd(a, b));
            CHECK(a * result.x + b * result.y == result.gcd);
        }

    for (int modulus = 1; modulus <= 100; ++modulus)
        for (int value = -100; value <= 100; ++value) {
            auto inverse = ninv_mod(value, modulus);
            bool exists = gcd(value, modulus) == 1;
            CHECK(bool(inverse) == exists);
            if (inverse) CHECK((value * *inverse % modulus + modulus) % modulus == 1 % modulus);
        }

    for (int modulus_a = 1; modulus_a <= 30; ++modulus_a)
        for (int modulus_b = 1; modulus_b <= 30; ++modulus_b)
            for (int a = 0; a < modulus_a; ++a)
                for (int b = 0; b < modulus_b; ++b) {
                    auto result = ncrt(a, modulus_a, b, modulus_b);
                    int brute = -1, limit = lcm(modulus_a, modulus_b);
                    for (int value = 0; value < limit; ++value)
                        if (value % modulus_a == a && value % modulus_b == b) { brute = value; break; }
                    CHECK(bool(result) == (brute >= 0));
                    if (result) CHECK(result->first == brute && result->second == limit);
                }

    mt19937_64 rng(0xA7A);
    for (int round = 0; round < 100000; ++round) {
        long long a = static_cast<long long>(rng() % 4'000'000'001ULL) - 2'000'000'000LL;
        long long b = static_cast<long long>(rng() % 4'000'000'001ULL) - 2'000'000'000LL;
        mint x = a, y = b;
        auto norm = [](long long value) {
            value %= mint::mod();
            return value < 0 ? value + mint::mod() : value;
        };
        CHECK(int(x + y) == norm(norm(a) + norm(b)));
        CHECK(int(x - y) == norm(norm(a) - norm(b)));
        CHECK(int(x * y) == static_cast<long long>(norm(a)) * norm(b) % mint::mod());
        int exponent = int(rng() % 1000);
        long long brute = 1, base = norm(a);
        for (int i = 0; i < exponent; ++i) brute = brute * base % mint::mod();
        CHECK(int(x.pow(exponent)) == brute);
        if (int(y)) CHECK(int(x / y * y) == int(x));
    }

    ncomb<mint> combinations(300);
    vector<vector<mint>> pascal(301, vector<mint>(301));
    pascal[0][0] = 1;
    for (int n = 1; n <= 300; ++n) {
        pascal[n][0] = pascal[n][n] = 1;
        for (int k = 1; k < n; ++k) pascal[n][k] = pascal[n - 1][k - 1] + pascal[n - 1][k];
    }
    for (int n = 0; n <= 300; ++n)
        for (int k = 0; k <= n; ++k) CHECK(combinations.choose(n, k) == pascal[n][k]);

    nsieve sieve(200000);
    for (int value = 1; value <= 200000; ++value) {
        bool prime = value >= 2;
        for (int divisor = 2; 1LL * divisor * divisor <= value; ++divisor)
            if (value % divisor == 0) { prime = false; break; }
        CHECK(sieve.prime(value) == prime);
        long long rebuilt = 1;
        for (auto [factor, exponent] : sieve.factor(value))
            for (int i = 0; i < exponent; ++i) rebuilt *= factor;
        CHECK(rebuilt == value);
        int coprime = 0;
        if (value <= 1000)
            for (int x = 1; x <= value; ++x) coprime += gcd(x, value) == 1;
        if (value <= 1000) CHECK(sieve.phi(value) == coprime);
    }
}
