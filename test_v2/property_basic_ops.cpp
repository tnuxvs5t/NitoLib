#include "common.hpp"

int main() {
    mt19937 rng(0xB451C0DEU);
    for (int trial = 0; trial < 500; ++trial) {
        int n = int(rng() % 100);
        vector<int> oracle(n);
        nvector<int> source(n), destination(n, 0);
        ndeque<int> other;
        for (int i = 0; i < n; ++i) {
            oracle[i] = source[i] = int(rng() % 31) - 15;
            other.pushr(int(rng() % 31) - 15);
        }

        nassign(destination, source);
        for (int i = 0; i < n; ++i)
            ntest(destination[i] == oracle[i]);

        int left = n ? int(rng() % (n + 1)) : 0;
        int right = left + (n ? int(rng() % (n - left + 1)) : 0);
        int fill = int(rng() % 21) - 10;
        nfill(nsub(destination, left, right), fill);
        fill_n(oracle.begin() + left, right - left, fill);
        for (int i = 0; i < n; ++i)
            ntest(destination[i] == oracle[i]);

        vector<int> other_before(n);
        for (int i = 0; i < n; ++i)
            other_before[i] = other[i];
        nswap_ranges(destination, other);
        for (int i = 0; i < n; ++i) {
            ntest(destination[i] == other_before[i]);
            ntest(other[i] == oracle[i]);
        }

        int needle = int(rng() % 31) - 15;
        int count = 0, first = npos, minimum = npos, maximum = npos;
        bool all_nonnegative = true, any_zero = false;
        for (int i = 0; i < n; ++i) {
            int value = source[i];
            if (value == needle) {
                ++count;
                if (first == npos)
                    first = i;
            }
            all_nonnegative &= value >= 0;
            any_zero |= value == 0;
            if (minimum == npos || value < source[minimum])
                minimum = i;
            if (maximum == npos || source[maximum] < value)
                maximum = i;
        }
        ntest(ncount(source, needle) == count);
        ntest(nfind_if(source, [=](int value) { return value == needle; }) == first);
        ntest(ncontains(source, needle) == (count != 0));
        ntest(nall_of(source, [](int value) { return value >= 0; }) == all_nonnegative);
        ntest(nany_of(source, [](int value) { return value == 0; }) == any_zero);
        ntest(nnone_of(source, [](int value) { return value == 0; }) == !any_zero);
        ntest(nargmin(source) == minimum && nargmax(source) == maximum);
        ntest(nsame(source, nall(source)));
    }
}
