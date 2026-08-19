#include "../src-v3/func.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    mt19937 rng(0xC0FFEE);
    for (nidx_t round = 0; round < 20000; ++round) {
        nidx_t n = nidx_t(rng() % 33), m = nidx_t(rng() % 25);
        vector<nidx_t> a(n), b(m);
        for (nidx_t& x : a) x = nidx_t(rng() % 2001) - 1000;
        for (nidx_t& x : b) x = nidx_t(rng() % 2001) - 1000;

        nidx_t left = n ? nidx_t(rng() % (n + 1)) : 0;
        nidx_t right = left + nidx_t(rng() % (n - left + 1));
        auto reversed = nreverse(nsub(nall(a), left, right));
        CHECK(reversed.len() == right - left);
        for (nidx_t i = 0; i < reversed.len(); ++i)
            CHECK(reversed[i] == a[right - 1 - i]);

        nidx_t q = nidx_t(rng() % 40);
        vector<nidx_t> indices(q);
        if (n)
            for (nidx_t& i : indices) i = nidx_t(rng() % n);
        else
            indices.clear();
        auto gathered = ngather(nall(a), nall(indices));
        for (nidx_t i = 0; i < gathered.len(); ++i)
            CHECK(gathered[i] == a[indices[i]]);

        auto transformed = nmap(nall(a), [](nidx_t x) { return 1LL * x * x - 3LL * x; });
        for (nidx_t i = 0; i < n; ++i)
            CHECK(transformed[i] == 1LL * a[i] * a[i] - 3LL * a[i]);

        auto zipped = nzip(nall(a), nall(b));
        CHECK(zipped.len() == min(n, m));
        for (nidx_t i = 0; i < zipped.len(); ++i)
            CHECK(get<0>(zipped[i]) == a[i] && get<1>(zipped[i]) == b[i]);

        auto product = nproduct(nall(a), nall(b));
        CHECK(product.len() == n * m);
        for (nidx_t i = 0; i < product.len(); ++i) {
            auto item = product[i];
            CHECK(item.first == a[i / m] && item.second == b[i % m]);
        }

        vector<nidx_t> permutation(n);
        iota(permutation.begin(), permutation.end(), 0);
        shuffle(permutation.begin(), permutation.end(), rng);
        vector<long long> payload(n);
        for (long long& x : payload) x = nidx_t(rng() % 100000);
        vector<nidx_t> inverse(n);
        for (nidx_t i = 0; i < n; ++i) inverse[permutation[i]] = i;
        auto function = nfunc_bind(nall(permutation), nall(payload),
                                   [&](nidx_t key) { return inverse[key]; });
        CHECK(function.len() == n);
        for (nidx_t i = 0; i < n; ++i) {
            CHECK(function.key(i) == permutation[i]);
            CHECK(function[i] == payload[i]);
            CHECK(function(permutation[i]) == payload[i]);
        }
    }
}
