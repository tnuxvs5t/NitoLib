#include "../src-v3/number.hpp"

void check(bool condition, const char* message) {
    if (!condition) throw runtime_error(message);
}

bool trial_prime(uint64_t value) {
    if (value < 2) return false;
    for (uint64_t divisor = 2; divisor * divisor <= value; ++divisor)
        if (value % divisor == 0) return false;
    return true;
}

void verify_factor(uint64_t value) {
    auto factors = nfactor(value);
    check(is_sorted(factors.begin(), factors.end()), "factor order");
    __uint128_t product = 1;
    for (uint64_t factor : factors) {
        check(nisprime(factor), "factor primality");
        product *= factor;
    }
    check(product == value, "factor product");
}

int main() {
    vector<uint64_t> primes{2, 3, 5, 37, 97, 1000000007ULL, 2305843009213693951ULL,
                            18446744073709551557ULL};
    vector<uint64_t> composites{0, 1, 4, 9, 341550071728321ULL,
                                3825123056546413051ULL, UINT64_MAX};
    for (auto value : primes) check(nisprime(value), "known prime");
    for (auto value : composites) check(!nisprime(value), "known composite");

    for (uint64_t value = 0; value < 200000; ++value)
        check(nisprime(value) == trial_prime(value), "trial primality");

    verify_factor(1);
    verify_factor(UINT64_MAX);
    verify_factor(1000000007ULL * 1000000009ULL);
    verify_factor(2305843009213693951ULL);

    mt19937_64 random(0xfa67022026ULL);
    for (nidx_t trial = 0; trial < 3000; ++trial) {
        uint64_t value = 1 + random() % 1000000000ULL;
        verify_factor(value);
    }
}
