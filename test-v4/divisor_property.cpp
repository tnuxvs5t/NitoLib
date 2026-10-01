#include "../src-v4/divisor.hpp"
#include "../src-v4/math.hpp"
#include "../src-v4/number.hpp"
#include "../src-v4/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

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
            auto divisors = ndivisors(sieve.factor(value));
            ranges::sort(divisors);
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
        if (size >= 12)
            CHECK(mu[1] == 1 && mu[2] == -1 && mu[4] == 0 && mu[6] == 1 && mu[12] == 0);
    }

    for (uint64_t value : vector<uint64_t>{1, 2, 36, 360,
                                           uint64_t(1000000007ULL) * 1000000009ULL,
                                           UINT64_MAX}) {
        auto factors = nfactor(value);
        auto powers = nfactor_powers(span(factors));
        auto divisors = ndivisors(powers);
        uint64_t count = 1;
        for (auto [prime, exponent] : powers) {
            CHECK(prime > 1 && exponent > 0);
            count *= uint64_t(exponent + 1);
        }
        CHECK(divisors.size() == count);
        ranges::sort(divisors);
        CHECK(ranges::adjacent_find(divisors) == divisors.end());
        CHECK(divisors.front() == 1 && divisors.back() == value);
        for (uint64_t d : divisors) CHECK(value % d == 0);
    }

    mt19937 rng(0xd17150);
    for (nidx_t round = 0; round < 1000; ++round) {
        nidx_t size = nidx_t(rng() % 150);
        vector<long long> source(size), divisors(size), multiples(size);
        for (auto& value : source) value = static_cast<long long>(rng() % 101) - 50;
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

    cout << "v4 divisor: sieve factor views, divisor enumeration and zeta transforms passed\n";
}
