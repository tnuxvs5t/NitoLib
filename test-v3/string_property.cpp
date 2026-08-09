#include "../src-v3/string.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    mt19937 rng(0x57A1A6);
    for (int round = 0; round < 20000; ++round) {
        int n = int(rng() % 55), m = int(rng() % 20);
        vector<int> sequence(n), pattern(m);
        for (int& value : sequence) value = int(rng() % 5);
        for (int& value : pattern) value = int(rng() % 5);

        auto prefix = nprefix(nall(sequence));
        auto z = nz(nall(sequence));
        for (int i = 0; i < n; ++i) {
            int expected_prefix = 0;
            for (int length = 1; length <= i; ++length)
                if (equal(sequence.begin(), sequence.begin() + length,
                          sequence.begin() + i + 1 - length)) expected_prefix = length;
            CHECK(prefix[i] == expected_prefix);
            int expected_z = 0;
            while (i + expected_z < n && sequence[expected_z] == sequence[i + expected_z])
                ++expected_z;
            CHECK(z[i] == expected_z);
        }

        vector<int> matches;
        for (int start = 0; start + m <= n; ++start)
            if (equal(pattern.begin(), pattern.end(), sequence.begin() + start))
                matches.push_back(start);
        CHECK(nkmp(nall(sequence), nall(pattern)) == matches);

        auto palindrome = nmanacher(nall(sequence));
        for (int center = 0; center < n; ++center) {
            int odd = 1;
            while (center - odd >= 0 && center + odd < n &&
                   sequence[center - odd] == sequence[center + odd]) ++odd;
            int even = 0;
            while (center - even - 1 >= 0 && center + even < n &&
                   sequence[center - even - 1] == sequence[center + even]) ++even;
            CHECK(palindrome.odd[center] == odd && palindrome.even[center] == even);
        }

        auto suffix = nsuffix_array(nall(sequence));
        vector<int> brute(n);
        iota(brute.begin(), brute.end(), 0);
        sort(brute.begin(), brute.end(), [&](int a, int b) {
            return lexicographical_compare(sequence.begin() + a, sequence.end(),
                                           sequence.begin() + b, sequence.end());
        });
        CHECK(suffix == brute);
        auto lcp = nlcp(nall(sequence), suffix);
        for (int i = 0; i + 1 < n; ++i) {
            int expected = 0;
            while (suffix[i] + expected < n && suffix[i + 1] + expected < n &&
                   sequence[suffix[i] + expected] == sequence[suffix[i + 1] + expected]) ++expected;
            CHECK(lcp[i] == expected);
        }
    }

    vector<int> periodic(1000, 7);
    auto suffix = nsuffix_array(nall(periodic));
    for (int i = 0; i < int(suffix.size()); ++i) CHECK(suffix[i] == int(suffix.size()) - 1 - i);
    auto lcp = nlcp(nall(periodic), suffix);
    for (int i = 0; i < int(lcp.size()); ++i) CHECK(lcp[i] == i + 1);
}
