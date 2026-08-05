#include "common.hpp"

int main() {
    mt19937 rng(31622776);
    for (int trial = 0; trial < 1000; ++trial) {
        int n = int(rng() % 50);
        string text(n, 'a');
        for (char& c : text)
            c = char('a' + rng() % 4);

        auto palindrome = nmanacher(text);
        for (int left = 0; left <= n; ++left)
            for (int right = left; right <= n; ++right) {
                bool expected = true;
                for (int i = 0; i < (right - left) / 2; ++i)
                    expected = expected && text[left + i] == text[right - 1 - i];
                ntest(palindrome.pal(left, right) == expected);
            }

        auto suffix = nsuffix_array(text);
        vector<int> expected_suffix(n);
        iota(expected_suffix.begin(), expected_suffix.end(), 0);
        sort(expected_suffix.begin(), expected_suffix.end(),
             [&](int a, int b) { return text.substr(a) < text.substr(b); });
        for (int i = 0; i < n; ++i)
            ntest(suffix[i] == expected_suffix[i]);

        auto lcp = nlcp_array(text, suffix);
        for (int i = 1; i < n; ++i) {
            int expected = 0;
            while (suffix[i] + expected < n && suffix[i - 1] + expected < n &&
                   text[suffix[i] + expected] == text[suffix[i - 1] + expected])
                ++expected;
            ntest(lcp[i] == expected);
        }

        int m = int(rng() % 12);
        string pattern(m, 'a');
        for (char& c : pattern)
            c = char('a' + rng() % 4);
        vector<int> expected_match;
        for (int position = 0; position + m <= n; ++position)
            if (text.compare(position, m, pattern) == 0)
                expected_match.push_back(position);
        auto actual_match = nkmp_find(text, pattern);
        ntest(actual_match.len() == int(expected_match.size()));
        for (int i = 0; i < actual_match.len(); ++i)
            ntest(actual_match[i] == expected_match[i]);
    }
}
