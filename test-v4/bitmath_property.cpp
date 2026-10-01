#include "../src-v4/bitmath.hpp"
#include "../src-v4/math.hpp"
#include "../src-v4/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct no_div {
    long long value = 0;
    no_div& operator+=(no_div other) { value += other.value; return *this; }
    no_div& operator-=(no_div other) { value -= other.value; return *this; }
    no_div& operator*=(no_div other) { value *= other.value; return *this; }
    friend no_div operator+(no_div a, no_div b) { return a += b; }
    friend no_div operator-(no_div a, no_div b) { return a -= b; }
    friend no_div operator*(no_div a, no_div b) { return a *= b; }
    friend bool operator==(no_div, no_div) = default;
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
    nxor_basis<uint8_t> byte;
    CHECK(byte.insert(128) && byte.insert(255) && byte.maximize() == 255);
    CHECK(byte.kth(1) == optional<uint8_t>(127));

    mt19937_64 rng(0xb1752026);
    for (nidx_t round = 0; round < 700; ++round) {
        nidx_t n = nidx_t(rng() % 12);
        vector<uint64_t> values(n), span_values{0};
        nxor_basis<> basis, first, second;
        for (nidx_t i = 0; i < n; ++i) {
            values[i] = round & 1 ? rng() : rng() % 256;
            bool fresh = find(span_values.begin(), span_values.end(), values[i]) == span_values.end();
            CHECK(basis.insert(values[i]) == fresh);
            (i < n / 2 ? first : second).insert(values[i]);
            nidx_t old = nlen(span_values);
            for (nidx_t j = 0; j < old; ++j) span_values.push_back(span_values[j] ^ values[i]);
        }
        ranges::sort(span_values);
        span_values.erase(ranges::unique(span_values).begin(), span_values.end());
        CHECK(span_values.size() == (size_t(1) << basis.rank()));
        CHECK(basis.maximize() == span_values.back());
        first.merge(second);
        CHECK(first.rank() == basis.rank() && first.ordered() == basis.ordered());
        for (size_t k = 0; k < span_values.size(); ++k)
            CHECK(basis.kth(uint64_t(k)) == optional<uint64_t>(span_values[k]));
        CHECK(!basis.kth(uint64_t(span_values.size())));
        for (nidx_t query = 0; query < 12; ++query) {
            uint64_t seed = rng(), best = 0;
            for (uint64_t value : span_values) best = max(best, seed ^ value);
            CHECK(basis.contains(seed) == binary_search(span_values.begin(), span_values.end(), seed));
            CHECK(basis.maximize(seed) == best);
        }
    }

    using mint = nmodint<998244353>;
    for (nidx_t round = 0; round < 500; ++round) {
        nidx_t n = round == 0 ? 0 : nidx_t(1) << (rng() % 7);
        vector<mint> a(n), b(n), subset(n), superset(n);
        for (nidx_t i = 0; i < n; ++i) a[i] = rng(), b[i] = rng();
        for (nidx_t i = 0; i < n; ++i)
            for (nidx_t j = 0; j < n; ++j) {
                if ((i & j) == j) subset[i] += a[j];
                if ((i & j) == i) superset[i] += a[j];
            }
        auto original = a;
        nsubset_zeta(span(a));
        CHECK(a == subset);
        nsubset_zeta(nall(a), true);
        CHECK(a == original);
        nsuperset_zeta(a);
        CHECK(a == superset);
        nsuperset_zeta(a, true);
        CHECK(a == original);
        nxor_transform(a);
        nxor_transform(a, true);
        CHECK(a == original);

        auto verify = [&]<nbit_operation Operation> {
            vector<mint> expected(n);
            for (nidx_t i = 0; i < n; ++i)
                for (nidx_t j = 0; j < n; ++j) {
                    nidx_t at = Operation == nbit_operation::bit_and ? i & j
                              : Operation == nbit_operation::bit_or ? i | j : i ^ j;
                    expected[at] += original[i] * b[j];
                }
            CHECK(nbit_convolution<Operation>(nall(original), span(b)) == expected);
        };
        verify.operator()<nbit_operation::bit_and>();
        verify.operator()<nbit_operation::bit_or>();
        verify.operator()<nbit_operation::bit_xor>();
    }

    vector<long long> left{1, 2, 3, 4}, right{5, 6, 7, 8};
    CHECK(nbit_convolution<nbit_operation::bit_or>(left, right) ==
          vector<long long>({5, 28, 43, 184}));
    vector<no_div> ring_left{{1}, {2}}, ring_right{{3}, {4}};
    CHECK(nbit_convolution<nbit_operation::bit_or>(ring_left, ring_right) ==
          vector<no_div>({{3}, {18}}));

    cout << "v4 bitmath: XOR basis, subset transforms and bit convolution passed\n";
}
