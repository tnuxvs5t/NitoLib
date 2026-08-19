#include "../src-v3/string.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    mt19937 rng(0x57A1A6);
    for (nidx_t round = 0; round < 20000; ++round) {
        nidx_t n = nidx_t(rng() % 55), m = nidx_t(rng() % 20);
        vector<nidx_t> sequence(n), pattern(m);
        for (nidx_t& value : sequence) value = nidx_t(rng() % 5);
        for (nidx_t& value : pattern) value = nidx_t(rng() % 5);

        auto prefix = nprefix_function(nall(sequence));
        auto z = nz(nall(sequence));
        for (nidx_t i = 0; i < n; ++i) {
            nidx_t expected_prefix = 0;
            for (nidx_t length = 1; length <= i; ++length)
                if (equal(sequence.begin(), sequence.begin() + length,
                          sequence.begin() + i + 1 - length)) expected_prefix = length;
            CHECK(prefix[i] == expected_prefix);
            nidx_t expected_z = 0;
            while (i + expected_z < n && sequence[expected_z] == sequence[i + expected_z])
                ++expected_z;
            CHECK(z[i] == expected_z);
        }

        vector<nidx_t> matches;
        for (nidx_t start = 0; start + m <= n; ++start)
            if (equal(pattern.begin(), pattern.end(), sequence.begin() + start))
                matches.push_back(start);
        CHECK(nkmp(nall(sequence), nall(pattern)) == matches);

        auto palindrome = nmanacher(nall(sequence));
        for (nidx_t center = 0; center < n; ++center) {
            nidx_t odd = 1;
            while (center - odd >= 0 && center + odd < n &&
                   sequence[center - odd] == sequence[center + odd]) ++odd;
            nidx_t even = 0;
            while (center - even - 1 >= 0 && center + even < n &&
                   sequence[center - even - 1] == sequence[center + even]) ++even;
            CHECK(palindrome.odd[center] == odd && palindrome.even[center] == even);
        }

        auto suffix = nsuffix_array(nall(sequence));
        vector<nidx_t> brute(n);
        iota(brute.begin(), brute.end(), 0);
        sort(brute.begin(), brute.end(), [&](nidx_t a, nidx_t b) {
            return lexicographical_compare(sequence.begin() + a, sequence.end(),
                                           sequence.begin() + b, sequence.end());
        });
        CHECK(suffix == brute);
        auto lcp = nlcp(nall(sequence), suffix);
        for (nidx_t i = 0; i + 1 < n; ++i) {
            nidx_t expected = 0;
            while (suffix[i] + expected < n && suffix[i + 1] + expected < n &&
                   sequence[suffix[i] + expected] == sequence[suffix[i + 1] + expected]) ++expected;
            CHECK(lcp[i] == expected);
        }
    }

    vector<nidx_t> periodic(1000, 7);
    auto suffix = nsuffix_array(nall(periodic));
    for (nidx_t i = 0; i < nidx_t(suffix.size()); ++i) CHECK(suffix[i] == nidx_t(suffix.size()) - 1 - i);
    auto lcp = nlcp(nall(periodic), suffix);
    for (nidx_t i = 0; i < nidx_t(lcp.size()); ++i) CHECK(lcp[i] == i + 1);
}
