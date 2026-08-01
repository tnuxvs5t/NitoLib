#include "common.hpp"

int main() {
    mt19937 rng(20260801);
    for (int trial = 0; trial < 500; ++trial) {
        int n = int(rng() % 100);
        vector<int> expected(n);
        ndeque<int> actual;
        for (int& x : expected) {
            x = int(rng() % 41) - 20;
            actual.pushr(x);
        }
        sort(expected.begin(), expected.end());
        nsort(actual);
        ntest(actual.len() == n);
        for (int i = 0; i < n; ++i)
            ntest(actual[i] == expected[i]);
    }

    for (int trial = 0; trial < 300; ++trial) {
        int n = 1 + int(rng() % 80);
        nvector<int> storage(2 * n, 1000000);
        vector<int> expected(n);
        for (int i = 0; i < n; ++i)
            storage[2 * i] = expected[i] = int(rng());
        auto even = nstride(storage, 0, n, 2);
        nsort(even, ngreater<>{});
        sort(expected.begin(), expected.end(), greater<>{});
        for (int i = 0; i < n; ++i) {
            ntest(storage[2 * i] == expected[i]);
            ntest(storage[2 * i + 1] == 1000000);
        }
    }
}
