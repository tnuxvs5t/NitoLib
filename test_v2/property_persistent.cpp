#include "common.hpp"

int main() {
    mt19937 rng(14142135);
    constexpr int n = 64;
    nvector<long long> initial(n, 0);
    npersistent_seg<long long> tree(initial);
    vector<vector<long long>> versions(1, vector<long long>(n));

    for (int operation = 0; operation < 5000; ++operation) {
        int base = int(rng() % versions.size());
        int index = int(rng() % n);
        long long value = int(rng() % 2001) - 1000;
        int created = tree.set(base, index, value);
        auto snapshot = versions[base];
        snapshot[index] = value;
        versions.push_back(move(snapshot));
        ntest(created == int(versions.size()) - 1);

        for (int trial = 0; trial < 5; ++trial) {
            int version = int(rng() % versions.size());
            int left = int(rng() % (n + 1));
            int right = int(rng() % (n + 1));
            if (left > right)
                swap(left, right);
            long long expected = accumulate(versions[version].begin() + left,
                                            versions[version].begin() + right, 0LL);
            ntest(tree.fold(version, left, right) == expected);
        }
    }
}
