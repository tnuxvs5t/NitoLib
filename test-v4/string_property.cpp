#include "../src-v4/string.hpp"
#include "../src-v4/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    mt19937 rng(0x57a1a6);
    for (nidx_t round = 0; round < 14000; ++round) {
        nidx_t n = nidx_t(rng() % 55), m = nidx_t(rng() % 20);
        vector<nidx_t> sequence(n), pattern(m);
        for (auto& value : sequence) value = rng() % 5;
        for (auto& value : pattern) value = rng() % 5;

        auto prefix = nprefix_function(nall(sequence));
        auto z = nz(nall(sequence));
        for (nidx_t i = 0; i < n; ++i) {
            nidx_t border = 0;
            for (nidx_t length = 1; length <= i; ++length)
                if (equal(sequence.begin(), sequence.begin() + length,
                           sequence.begin() + i + 1 - length)) border = length;
            CHECK(prefix[i] == border);
            nidx_t matches = 0;
            while (i + matches < n && sequence[matches] == sequence[i + matches]) ++matches;
            CHECK(z[i] == matches);
        }

        vector<nidx_t> expected_matches;
        for (nidx_t start = 0; start + m <= n; ++start)
            if (equal(pattern.begin(), pattern.end(), sequence.begin() + start))
                expected_matches.push_back(start);
        CHECK(nkmp(nall(sequence), nall(pattern)) == expected_matches);

        auto radii = nmanacher(nall(sequence));
        for (nidx_t center = 0; center < n; ++center) {
            nidx_t odd = 1;
            while (center - odd >= 0 && center + odd < n &&
                   sequence[center - odd] == sequence[center + odd]) ++odd;
            nidx_t even = 0;
            while (center - even - 1 >= 0 && center + even < n &&
                   sequence[center - even - 1] == sequence[center + even]) ++even;
            CHECK(radii.odd[center] == odd && radii.even[center] == even);
        }

        auto suffix = nsuffix_array(nall(sequence));
        vector<nidx_t> brute(n);
        iota(brute.begin(), brute.end(), 0);
        ranges::sort(brute, [&](nidx_t left, nidx_t right) {
            return lexicographical_compare(sequence.begin() + left, sequence.end(),
                                           sequence.begin() + right, sequence.end());
        });
        CHECK(suffix == brute);
        auto lcp = nlcp(nall(sequence), suffix);
        for (nidx_t i = 0; i + 1 < n; ++i) {
            nidx_t common = 0;
            while (suffix[i] + common < n && suffix[i + 1] + common < n &&
                   sequence[suffix[i] + common] == sequence[suffix[i + 1] + common]) ++common;
            CHECK(lcp[i] == common);
        }
    }

    vector<int> empty;
    CHECK(nprefix_function(empty).empty() && nz(empty).empty());
    CHECK(nkmp(empty, empty) == vector<nidx_t>{0});
    CHECK(nsuffix_array(empty).empty() && nlcp(empty, {}).empty());
    vector<nidx_t> periodic(1000, 7);
    auto suffix = nsuffix_array(nall(periodic));
    for (nidx_t i = 0; i < nidx_t(suffix.size()); ++i)
        CHECK(suffix[i] == nidx_t(suffix.size()) - 1 - i);
    auto lcp = nlcp(nall(periodic), suffix);
    for (nidx_t i = 0; i < nidx_t(lcp.size()); ++i) CHECK(lcp[i] == i + 1);

    cout << "v4 string: prefix/Z/KMP, Manacher, suffix array and LCP passed\n";
}
