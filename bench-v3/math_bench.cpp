#include "../src-v3/math.hpp"
#include "../src-v3/divisor.hpp"
#include "../src-v3/bitmath.hpp"
#include "../src-v3/poly.hpp"
#include "../src-v3/linear.hpp"
#include "../src-v3/recurrence.hpp"

using mint = nmodint<998244353>;

int main() {
    uint64_t checksum = 0;
    auto run = [&](const char* label, auto work) {
        auto start = chrono::steady_clock::now();
        work();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start);
        cout << label << " ms=" << elapsed.count() << " checksum=" << checksum << '\n';
    };
    run("sieve_phi_mu n=1000000", [&] {
        nsieve sieve(1000000);
        auto phi = sieve.phi_table();
        auto mu = sieve.mu_table();
        for (nidx_t i = 1; i < nlen(phi); ++i) checksum += uint64_t(phi[i]);
        for (nidx_t i = 1; i < nlen(mu); ++i) checksum += uint64_t(mu[i] + 1);
        if (sieve.primes.size() != 78498 || phi[1000000] != 400000 || mu[1000000] != 0) abort();
    });
    run("divisor_transforms n=200000", [&] {
        vector<long long> values(200001);
        for (nidx_t i = 1; i < nlen(values); ++i) values[i] = i % 17;
        auto original = values;
        ndivisor_zeta(values);
        for (auto value : values) checksum += uint64_t(value);
        ndivisor_mobius(values);
        if (values != original) abort();
        nmultiple_zeta(values);
        for (auto value : values) checksum += uint64_t(value);
        nmultiple_mobius(values);
        if (values != original) abort();
    });
    run("floor_sum queries=100000", [&] {
        for (long long i = 1; i <= 100000; ++i)
            checksum += uint64_t(nfloor_sum(1000000000, 1000000007, 48271 * i, -i));
    });
    run("xor_basis inserts=100000", [&] {
        nxor_basis<> basis;
        uint64_t state = 1;
        for (nidx_t i = 0; i < 100000; ++i) {
            state = state * 6364136223846793005ULL + 1;
            basis.insert(state);
        }
        if (basis.rank() != 64) abort();
        checksum += basis.maximize();
    });
    run("bit_convolution xor n=262144", [&] {
        vector<mint> a(1 << 18), b(1 << 18);
        mint sum_a = 0, sum_b = 0;
        for (nidx_t i = 0; i < nlen(a); ++i) {
            a[i] = i % 97;
            b[i] = i % 89;
            sum_a += a[i];
            sum_b += b[i];
        }
        auto product = nbit_convolution<nbit_operation::bit_xor>(a, b);
        mint sum = 0;
        for (mint value : product) sum += value, checksum += uint64_t(value.value);
        if (sum != sum_a * sum_b) abort();
    });
    run("interpolation n=2048 queries=100", [&] {
        ncomb<mint> combinations(2047);
        vector<mint> values(2048);
        for (nidx_t i = 0; i < nlen(values); ++i) values[i] = mint(i).pow(17) + mint(3 * i) + mint(9);
        for (nidx_t i = 0; i < 100; ++i) {
            mint point = 1000000 + i;
            mint result = nlagrange_consecutive(values, point, combinations);
            if (result != point.pow(17) + mint(3) * point + mint(9)) abort();
            checksum += uint64_t(result.value);
        }
    });
    run("gf2_solve rows=512 variables=512", [&] {
        constexpr nidx_t n = 512, words = n / 64;
        nmatrix<uint64_t> coefficients(n, words);
        vector<unsigned char> right(n);
        for (nidx_t i = 0; i < n; ++i) {
            coefficients(i, i / 64) = uint64_t(1) << (i % 64);
            right[i] = i % 2;
        }
        uint64_t state = 7;
        for (nidx_t i = 0; i < 10000; ++i) {
            state = state * 6364136223846793005ULL + 1;
            nidx_t a = nidx_t(state % n), b = nidx_t((state >> 32) % n);
            if (a == b) continue;
            for (nidx_t w = 0; w < words; ++w) coefficients(a, w) ^= coefficients(b, w);
            right[a] ^= right[b];
        }
        auto solution = ngf2_solve(coefficients, n, right);
        if (!solution.consistent || solution.rank != n || solution.basis.rows) abort();
        for (uint64_t word : solution.particular) {
            if (word != 0xaaaaaaaaaaaaaaaaULL) abort();
            checksum += word;
        }
    });
    run("recurrence bm_terms=512 order=64 index=1e18", [&] {
        constexpr nidx_t k = 64, n = 512;
        constexpr uint64_t index = 1000000000000000000ULL;
        vector<mint> sequence(n), powers(k, mint(1));
        for (nidx_t i = 0; i < n; ++i)
            for (nidx_t j = 0; j < k; ++j) {
                sequence[i] += powers[j];
                powers[j] *= mint(j + 1);
            }
        auto coefficients = nberlekamp_massey(sequence);
        if (nlen(coefficients) != k) abort();
        auto value = nlinear_recurrence(sequence, coefficients, index);
        mint expected = 0;
        for (nidx_t j = 0; j < k; ++j) expected += mint(j + 1).pow(index);
        if (value != expected) abort();
        checksum += uint64_t(value.value);
    });
}
