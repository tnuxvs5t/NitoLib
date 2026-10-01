#include "../src-v4/number.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

bool trial_prime(uint64_t value) {
    if (value < 2) return false;
    for (uint64_t divisor = 2; divisor * divisor <= value; ++divisor)
        if (value % divisor == 0) return false;
    return true;
}

void check_factorization(uint64_t value) {
    auto factors = nfactor(value);
    CHECK(ranges::is_sorted(factors));
    __uint128_t product = 1;
    for (uint64_t factor : factors) {
        CHECK(nisprime(factor));
        product *= factor;
    }
    CHECK(product == value);
}

int main() {
    CHECK(naddmod64(1, 1, 7) == 2);
    CHECK(naddmod64(UINT64_MAX - 1, UINT64_MAX - 1, UINT64_MAX) == UINT64_MAX - 2);
    CHECK(naddmod64(0, 0, 1) == 0);
    CHECK(nmulmod64(UINT64_MAX, UINT64_MAX, 97) ==
          uint64_t(__uint128_t(UINT64_MAX) * UINT64_MAX % 97));
    CHECK(npowmod64(UINT64_MAX, 0, 1) == 0);
    CHECK(npowmod64(7, 100, 1009) == 227);

    vector<uint64_t> primes{2, 3, 5, 37, 97, 1000000007ULL,
                            2305843009213693951ULL, 18446744073709551557ULL};
    vector<uint64_t> composites{0, 1, 4, 9, 341550071728321ULL,
                                3825123056546413051ULL, UINT64_MAX};
    for (uint64_t value : primes) CHECK(nisprime(value));
    for (uint64_t value : composites) CHECK(!nisprime(value));
    for (uint64_t value = 0; value < 200000; ++value)
        CHECK(nisprime(value) == trial_prime(value));

    for (uint64_t value : vector<uint64_t>{1, 2, 4, 12,
                                           uint64_t(1000000007ULL) * 1000000007ULL,
                                           uint64_t(1000000007ULL) * 1000000009ULL,
                                           UINT64_MAX})
        check_factorization(value);

    for (uint64_t value : vector<uint64_t>{9, 25, 49,
                                           uint64_t(1000000007ULL) * 1000000009ULL,
                                           UINT64_MAX}) {
        uint64_t divisor = npollard(value);
        CHECK(divisor > 1 && divisor < value && value % divisor == 0);
    }

    mt19937_64 rng(0x5eed1234ULL);
    for (nidx_t round = 0; round < 4000; ++round)
        check_factorization(1 + rng() % 1000000000000ULL);

    cout << "v4 number: uint64 modular arithmetic, primality and Pollard factorization passed\n";
}
