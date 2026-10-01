#include "../src-v3/permutation.hpp"
#include "../src-v3/func.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    array<nidx_t, 5> fixed{0, 1, 2, 3, 4};
    nrotate(fixed, 2);
    CHECK((fixed == array<nidx_t, 5>{2, 3, 4, 0, 1}));
    nrotate(fixed, 1, 3, 5);
    CHECK((fixed == array<nidx_t, 5>{2, 0, 1, 3, 4}));
    nrotate(fixed, 0);
    nrotate(fixed, nidx_t(fixed.size()));
    CHECK((fixed == array<nidx_t, 5>{2, 0, 1, 3, 4}));
    vector<nidx_t> empty;
    nrotate(empty, 0);
    nrotate(empty, 0, 0, 0);
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
        uint64_t expected_rank = 0;
        do {
            CHECK(npermutation_rank(permutation) == expected_rank);
            CHECK(npermutation_unrank(n, expected_rank) == permutation);
            ++expected_rank;
        } while (next_permutation(permutation.begin(), permutation.end()));
        uint64_t factorial = 1;
        for (nidx_t i = 1; i <= n; ++i) factorial *= i;
        CHECK(expected_rank == factorial);
    }

    mt19937_64 rng(0x9E7A2026);
    for (nidx_t trial = 0; trial < 5000; ++trial) {
        nidx_t n = nidx_t(rng() % 61);
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

        nidx_t left = nidx_t(rng() % (n + 1));
        nidx_t right = left + nidx_t(rng() % (n - left + 1));
        middle = left + nidx_t(rng() % (right - left + 1));
        expected = input, actual = input;
        rotate(expected.begin() + left, expected.begin() + middle, expected.begin() + right);
        nrotate(actual, left, middle, right);
        CHECK(actual == expected);

        middle = nidx_t(rng() % (n + 1));
        expected.assign(input.rbegin(), input.rend());
        rotate(expected.begin(), expected.begin() + middle, expected.end());
        actual = input;
        nrotate(nreverse(nall(actual)), middle);
        CHECK((vector<nidx_t>(actual.rbegin(), actual.rend()) == expected));
    }

    uint64_t factorial20 = 1;
    for (uint64_t i = 1; i <= 20; ++i) factorial20 *= i;
    vector<nidx_t> descending20(20);
    iota(descending20.rbegin(), descending20.rend(), 0);
    CHECK(npermutation_rank(descending20) == factorial20 - 1);
    CHECK(npermutation_unrank(20, factorial20 - 1) == descending20);
    vector<nidx_t> ascending21(21);
    iota(ascending21.begin(), ascending21.end(), 0);
    CHECK(npermutation_rank(ascending21) == 0);
    CHECK(npermutation_unrank(21, 0ULL) == ascending21);
    swap(ascending21[19], ascending21[20]);
    CHECK(npermutation_rank(ascending21) == 1);
    CHECK(npermutation_unrank(21, 1ULL) == ascending21);
    for (nidx_t trial = 0; trial < 3000; ++trial) {
        nidx_t n = nidx_t(rng() % 21);
        vector<nidx_t> permutation(n);
        iota(permutation.begin(), permutation.end(), 0);
        shuffle(permutation.begin(), permutation.end(), rng);
        CHECK(npermutation_unrank(n, npermutation_rank(permutation)) == permutation);
    }

    using u128 = __uint128_t;
    u128 factorial25 = 1;
    for (nidx_t i = 1; i <= 25; ++i) factorial25 *= u128(i);
    vector<nidx_t> descending25(25);
    iota(descending25.rbegin(), descending25.rend(), 0);
    CHECK(npermutation_rank<u128>(descending25) == factorial25 - 1);
    CHECK(npermutation_unrank(25, factorial25 - 1) == descending25);
    for (nidx_t trial = 0; trial < 1500; ++trial) {
        vector<nidx_t> permutation(25);
        iota(permutation.begin(), permutation.end(), 0);
        shuffle(permutation.begin(), permutation.end(), rng);
        CHECK(npermutation_unrank(25, npermutation_rank<u128>(permutation)) == permutation);
    }
}
