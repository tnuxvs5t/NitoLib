#include "common.hpp"

int main() {
    mt19937 rng(27182818);
    constexpr int n = 96;
    nvector<long long> values(n, 0);
    nlazy_addsum<long long> tree(values);

    for (int operation = 0; operation < 15000; ++operation) {
        int left = int(rng() % (n + 1));
        int right = int(rng() % (n + 1));
        if (left > right)
            swap(left, right);
        if (rng() & 1) {
            long long delta = int(rng() % 201) - 100;
            tree.apply(left, right, delta);
            for (int i = left; i < right; ++i)
                values[i] += delta;
        } else {
            long long expected = 0;
            for (int i = left; i < right; ++i)
                expected += values[i];
            ntest(tree.fold(left, right) == expected);
        }
        if (operation % 97 == 0) {
            int index = int(rng() % n);
            long long value = int(rng() % 1001) - 500;
            values[index] = value;
            tree.set(index, value);
        }
    }
}
