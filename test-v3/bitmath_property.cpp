#include "../src-v3/bitmath.hpp"
#include "../src-v3/math.hpp"
#include "../src-v3/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x "\n"; abort(); } } while (false)
using mint = nmodint<998244353>;

struct division_free {
    long long value = 0;
    division_free& operator+=(division_free other) { value += other.value; return *this; }
    division_free& operator-=(division_free other) { value -= other.value; return *this; }
    division_free& operator*=(division_free other) { value *= other.value; return *this; }
    friend division_free operator*(division_free a, division_free b) { return {a.value * b.value}; }
    friend bool operator==(division_free, division_free) = default;
};

int main() {
    nxor_basis<> empty;
    CHECK(empty.rank() == 0 && empty.contains(0) && !empty.contains(1));
    CHECK(empty.kth(0) == optional<uint64_t>(0) && !empty.kth(1));
    CHECK(!empty.insert(0));
    nxor_basis<> full;
    for (nidx_t bit = 0; bit < 64; ++bit) CHECK(full.insert(uint64_t(1) << bit));
    CHECK(full.rank() == 64 && full.maximize() == UINT64_MAX);
    CHECK(full.kth(UINT64_MAX) == optional<uint64_t>(UINT64_MAX));
    CHECK(full.contains(UINT64_MAX) && !full.insert(UINT64_MAX));
    full.merge(full);
    CHECK(full.rank() == 64);
    nxor_basis<uint8_t> byte;
    CHECK(byte.insert(128) && byte.insert(255) && byte.maximize() == 255);
    CHECK(byte.kth(1) == optional<uint8_t>(127));

    mt19937_64 rng(0xb1752026);
    for (nidx_t trial = 0; trial < 500; ++trial) {
        nidx_t n = nidx_t(rng() % 11);
        vector<uint64_t> values(n), expected{0};
        nxor_basis<> basis, first, second;
        for (nidx_t i = 0; i < n; ++i) {
            values[i] = trial & 1 ? rng() : rng() % 256;
            bool new_value = find(expected.begin(), expected.end(), values[i]) == expected.end();
            CHECK(basis.insert(values[i]) == new_value);
            (i < n / 2 ? first : second).insert(values[i]);
            nidx_t old = nlen(expected);
            for (nidx_t j = 0; j < old; ++j) expected.push_back(expected[j] ^ values[i]);
        }
        sort(expected.begin(), expected.end());
        expected.erase(unique(expected.begin(), expected.end()), expected.end());
        CHECK(expected.size() == (size_t(1) << basis.rank()));
        CHECK(basis.maximize() == expected.back());
        first.merge(second);
        CHECK(first.rank() == basis.rank() && first.ordered() == basis.ordered());
        auto reduced = basis.ordered();
        for (size_t k = 0; k < expected.size(); ++k) {
            uint64_t actual = 0;
            for (nidx_t bit = 0; bit < nlen(reduced); ++bit)
                if ((k >> bit) & 1) actual ^= reduced[bit];
            CHECK(actual == expected[k] && basis.contains(expected[k]));
        }
        CHECK(!basis.kth(expected.size()));
        for (nidx_t query = 0; query < 10; ++query) {
            uint64_t k = rng() % expected.size(), seed = rng(), maximum = 0;
            CHECK(basis.kth(k) == optional<uint64_t>(expected[k]));
            CHECK(basis.contains(seed) == binary_search(expected.begin(), expected.end(), seed));
            for (uint64_t value : expected) maximum = max(maximum, seed ^ value);
            CHECK(basis.maximize(seed) == maximum);
        }
    }

    for (nidx_t trial = 0; trial < 500; ++trial) {
        nidx_t n = trial == 0 ? 0 : nidx_t(1) << (rng() % 7);
        vector<mint> a(n), b(n), subset(n), superset(n);
        for (nidx_t i = 0; i < n; ++i) a[i] = rng(), b[i] = rng();
        for (nidx_t i = 0; i < n; ++i)
            for (nidx_t j = 0; j < n; ++j) {
                if ((i & j) == j) subset[i] += a[j];
                if ((i & j) == i) superset[i] += a[j];
            }
        auto actual = a;
        nsubset_zeta(span(actual));
        CHECK(actual == subset);
        nsubset_zeta(nall(actual), true);
        CHECK(actual == a);
        nsuperset_zeta(actual);
        CHECK(actual == superset);
        nsuperset_zeta(actual, true);
        CHECK(actual == a);
        nxor_transform(actual);
        nxor_transform(actual, true);
        CHECK(actual == a);
        auto verify = [&]<nbit_operation Operation>() {
            vector<mint> expected(n);
            for (nidx_t i = 0; i < n; ++i)
                for (nidx_t j = 0; j < n; ++j) {
                    nidx_t at = Operation == nbit_operation::bit_and ? (i & j)
                              : Operation == nbit_operation::bit_or ? (i | j) : (i ^ j);
                    expected[at] += a[i] * b[j];
                }
            CHECK(nbit_convolution<Operation>(nall(a), span(b)) == expected);
        };
        verify.operator()<nbit_operation::bit_and>();
        verify.operator()<nbit_operation::bit_or>();
        verify.operator()<nbit_operation::bit_xor>();
    }
    vector<long long> a{1, 2, 3, 4}, b{5, 6, 7, 8};
    CHECK(nbit_convolution<nbit_operation::bit_or>(a, b) == vector<long long>({5, 28, 43, 184}));
    vector<division_free> ring_a{{1}, {2}}, ring_b{{3}, {4}};
    CHECK(nbit_convolution<nbit_operation::bit_or>(ring_a, ring_b)
          == vector<division_free>({{3}, {18}}));
    CHECK(nbit_convolution<nbit_operation::bit_and>(ring_a, ring_b)
          == vector<division_free>({{13}, {8}}));
    using composite = nmodint<12>;
    vector<composite> ring{1, 2, 3, 4};
    auto original = ring;
    nsubset_zeta(ring);
    nsubset_zeta(ring, true);
    nsuperset_zeta(ring);
    nsuperset_zeta(ring, true);
    CHECK(ring == original);
}
