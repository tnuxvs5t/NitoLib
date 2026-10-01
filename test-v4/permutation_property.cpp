#include "../src-v4/permutation.hpp"
#include "../src-v4/func.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    array<nidx_t, 5> fixed{0, 1, 2, 3, 4};
    nrotate(fixed, 2);
    CHECK((fixed == array<nidx_t, 5>{2, 3, 4, 0, 1}));
    nrotate(fixed, 1, 3, 5);
    CHECK((fixed == array<nidx_t, 5>{2, 0, 1, 3, 4}));

    vector<nidx_t> empty;
    nrotate(empty, 0);
    CHECK(npermutation_rank(empty) == 0);
    CHECK(npermutation_unrank(0, 0ULL).empty());
    CHECK(npermutation_rank(array{1, 0, 2}) == 2);
    CHECK((npermutation_unrank(3, 5ULL) == vector<nidx_t>{2, 1, 0}));

    vector<nidx_t> keys{40, 10, 30}, values{1, 2, 3};
    auto function = nanchors(nall(keys), nall(values));
    nrotate(function, 1);
    CHECK((keys == vector<nidx_t>{40, 10, 30}));
    CHECK((values == vector<nidx_t>{2, 3, 1}));
    for (nidx_t i = 0; i < function.len(); ++i)
        CHECK(function[i] == function(function.key(i)));

    for (nidx_t n = 0; n <= 8; ++n) {
        vector<nidx_t> permutation(n);
        iota(permutation.begin(), permutation.end(), 0);
        uint64_t expected = 0;
        do {
            CHECK(npermutation_rank(permutation) == expected);
            CHECK(npermutation_unrank(n, expected) == permutation);
            ++expected;
        } while (next_permutation(permutation.begin(), permutation.end()));
    }

    mt19937_64 rng(0x9E7A2026);
    for (int trial = 0; trial < 5000; ++trial) {
        nidx_t n = nidx_t(rng() % 21);
        vector<nidx_t> input(n);
        iota(input.begin(), input.end(), 0);
        shuffle(input.begin(), input.end(), rng);
        nidx_t middle = nidx_t(rng() % (n + 1));
        auto expected = input, actual = input;
        rotate(expected.begin(), expected.begin() + middle, expected.end());
        nrotate(nall(actual), middle);
        CHECK(actual == expected);
        nrotate(nall(actual), n - middle);
        CHECK(actual == input);

        CHECK(npermutation_unrank(n, npermutation_rank(input)) == input);
    }

    using u128 = __uint128_t;
    u128 factorial = 1;
    for (nidx_t i = 1; i <= 25; ++i) factorial *= u128(i);
    vector<nidx_t> descending(25);
    iota(descending.rbegin(), descending.rend(), 0);
    CHECK(npermutation_rank<u128>(descending) == factorial - 1);
    CHECK(npermutation_unrank(25, factorial - 1) == descending);

    cout << "v4 permutation: rotation and rank/unrank passed\n";
}
