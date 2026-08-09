#include "../src-v3/func.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    mt19937 rng(0xC0FFEE);
    for (int round = 0; round < 20000; ++round) {
        int n = int(rng() % 33), m = int(rng() % 25);
        vector<int> a(n), b(m);
        for (int& x : a) x = int(rng() % 2001) - 1000;
        for (int& x : b) x = int(rng() % 2001) - 1000;

        int left = n ? int(rng() % (n + 1)) : 0;
        int right = left + int(rng() % (n - left + 1));
        auto reversed = nreverse(nsub(nall(a), left, right));
        CHECK(reversed.len() == right - left);
        for (int i = 0; i < reversed.len(); ++i)
            CHECK(reversed[i] == a[right - 1 - i]);

        int q = int(rng() % 40);
        vector<int> indices(q);
        if (n)
            for (int& i : indices) i = int(rng() % n);
        else
            indices.clear();
        auto gathered = ngather(nall(a), nall(indices));
        for (int i = 0; i < gathered.len(); ++i)
            CHECK(gathered[i] == a[indices[i]]);

        auto transformed = nmap(nall(a), [](int x) { return 1LL * x * x - 3LL * x; });
        for (int i = 0; i < n; ++i)
            CHECK(transformed[i] == 1LL * a[i] * a[i] - 3LL * a[i]);

        auto zipped = nzip(nall(a), nall(b));
        CHECK(zipped.len() == min(n, m));
        for (int i = 0; i < zipped.len(); ++i)
            CHECK(get<0>(zipped[i]) == a[i] && get<1>(zipped[i]) == b[i]);

        auto product = nproduct(nall(a), nall(b));
        CHECK(product.len() == n * m);
        for (int i = 0; i < product.len(); ++i) {
            auto item = product[i];
            CHECK(item.first == a[i / m] && item.second == b[i % m]);
        }

        vector<int> permutation(n);
        iota(permutation.begin(), permutation.end(), 0);
        shuffle(permutation.begin(), permutation.end(), rng);
        vector<long long> payload(n);
        for (long long& x : payload) x = int(rng() % 100000);
        vector<int> inverse(n);
        for (int i = 0; i < n; ++i) inverse[permutation[i]] = i;
        auto function = nfunc_bind(nall(permutation), nall(payload),
                                   [&](int key) { return inverse[key]; });
        CHECK(function.len() == n);
        for (int i = 0; i < n; ++i) {
            CHECK(function.key(i) == permutation[i]);
            CHECK(function[i] == payload[i]);
            CHECK(function(permutation[i]) == payload[i]);
        }
    }
}
