#include "../src-v3/divisor.hpp"
#include "../src-v3/math.hpp"
#include "../src-v3/number.hpp"
#include "../src-v3/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    for (nidx_t size : {0, 1, 2, 3000}) {
        nsieve sieve(size);
        auto phi = sieve.phi_table();
        auto mu = sieve.mu_table();
        CHECK(nlen(phi) == size + 1 && nlen(mu) == size + 1);
        CHECK(phi[0] == 0 && mu[0] == 0);
        for (nidx_t value = 1; value <= size; ++value) {
            nidx_t coprime = 0;
            vector<nidx_t> expected;
            for (nidx_t d = 1; d <= value; ++d) {
                coprime += gcd(d, value) == 1;
                if (value % d == 0) expected.push_back(d);
            }
            CHECK(phi[value] == coprime && sieve.phi(value) == coprime);
            auto factors = sieve.factor(value);
            auto divisors = ndivisors(factors);
            sort(divisors.begin(), divisors.end());
            CHECK(divisors == expected);
            nidx_t mu_sum = 0, phi_sum = 0;
            for (nidx_t d : divisors) mu_sum += mu[d], phi_sum += phi[d];
            CHECK(mu_sum == (value == 1) && phi_sum == value);
            if (value > 1) {
                nidx_t least = 2;
                while (value % least) ++least;
                CHECK(sieve.least[value] == least);
            }
        }
        if (size >= 12) CHECK(mu[1] == 1 && mu[2] == -1 && mu[4] == 0 && mu[6] == 1 && mu[12] == 0);
    }
    for (uint64_t value : array<uint64_t, 6>{1, 2, 36, 360, 1000000007ULL * 1000000009ULL, UINT64_MAX}) {
        auto factors = nfactor(value);
        auto powers = nfactor_powers(span(factors));
        auto divisors = ndivisors(powers);
        uint64_t expected_count = 1;
        for (auto [prime, exponent] : powers) {
            CHECK(prime > 1 && exponent > 0);
            expected_count *= uint64_t(exponent + 1);
        }
        CHECK(divisors.size() == expected_count);
        sort(divisors.begin(), divisors.end());
        CHECK(adjacent_find(divisors.begin(), divisors.end()) == divisors.end());
        CHECK(divisors.front() == 1 && divisors.back() == value);
        for (uint64_t d : divisors) CHECK(value % d == 0);
    }

    mt19937 rng(0xd17150);
    for (nidx_t trial = 0; trial < 1000; ++trial) {
        nidx_t size = nidx_t(rng() % 150);
        vector<long long> source(size);
        for (auto& value : source) value = static_cast<long long>(rng() % 101) - 50;
        vector<long long> divisors(size), multiples(size);
        if (size) divisors[0] = multiples[0] = source[0];
        for (nidx_t i = 1; i < size; ++i)
            for (nidx_t j = 1; j < size; ++j) if (i % j == 0) {
                divisors[i] += source[j];
                multiples[j] += source[i];
            }
        auto actual = source;
        ndivisor_zeta(nall(actual));
        CHECK(actual == divisors);
        ndivisor_mobius(span(actual));
        CHECK(actual == source);
        nmultiple_zeta(actual);
        CHECK(actual == multiples);
        nmultiple_mobius(actual);
        CHECK(actual == source);
    }
    using mint = nmodint<12>;
    vector<mint> coefficients{7, 1, 2, 3, 4, 5, 6};
    auto original = coefficients;
    ndivisor_zeta(coefficients);
    ndivisor_mobius(coefficients);
    nmultiple_zeta(coefficients);
    nmultiple_mobius(coefficients);
    CHECK(coefficients == original);

    for (nidx_t trial = 0; trial < 500; ++trial) {
        vector<nidx_t> input(30);
        vector<long long> frequency(31), expected(31);
        for (auto& value : input) value = 1 + nidx_t(rng() % 30), ++frequency[value];
        for (nidx_t i = 0; i < nlen(input); ++i)
            for (nidx_t j = 0; j < i; ++j) ++expected[gcd(input[i], input[j])];
        nmultiple_zeta(frequency);
        for (nidx_t d = 1; d < nlen(frequency); ++d)
            frequency[d] = frequency[d] * (frequency[d] - 1) / 2;
        nmultiple_mobius(frequency);
        CHECK(frequency == expected);
    }
}
