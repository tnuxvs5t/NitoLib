#include "common.hpp"

int main() {
    mt19937 rng(31415926);
    constexpr int n = 127;
    nvector<long long> values(n, 0);
    nfenwick<long long> fenwick(n);
    nseg<long long> segment(n);

    for (int operation = 0; operation < 20000; ++operation) {
        if (rng() % 3) {
            int index = int(rng() % n);
            long long delta = int(rng() % 2001) - 1000;
            values[index] += delta;
            fenwick.add(index, delta);
            segment.set(index, values[index]);
        } else {
            int left = int(rng() % (n + 1));
            int right = int(rng() % (n + 1));
            if (left > right)
                swap(left, right);
            long long expected = 0;
            for (int i = left; i < right; ++i)
                expected += values[i];
            ntest(fenwick.fold(left, right) == expected);
            ntest(segment.fold(left, right) == expected);
        }
    }
}
