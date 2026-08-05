#include "common.hpp"

int main() {
    mt19937 rng(16180339);

    nqueue_agg<long long> queue;
    deque<long long> reference;
    for (int operation = 0; operation < 20000; ++operation) {
        if (reference.empty() || rng() % 3) {
            long long value = int(rng() % 2001) - 1000;
            reference.push_back(value);
            queue.push(value);
        } else {
            ntest(queue.front() == reference.front());
            ntest(queue.pop() == reference.front());
            reference.pop_front();
        }
        long long expected = accumulate(reference.begin(), reference.end(), 0LL);
        ntest(queue.fold() == expected && queue.len() == int(reference.size()));
    }

    constexpr int n = 40;
    ndsu dsu(n);
    vector<int> label(n);
    iota(label.begin(), label.end(), 0);
    for (int operation = 0; operation < 5000; ++operation) {
        int a = int(rng() % n), b = int(rng() % n);
        int old = label[b], replacement = label[a];
        dsu.merge(a, b);
        for (int& value : label)
            if (value == old)
                value = replacement;
        for (int trial = 0; trial < 20; ++trial) {
            int x = int(rng() % n), y = int(rng() % n);
            ntest(dsu.same(x, y) == (label[x] == label[y]));
        }
    }

    nrollback_dsu rollback(n);
    vector<pair<int, int>> edges;
    vector<pair<int, int>> checkpoints{{0, 0}};
    for (int block = 0; block < 200; ++block) {
        for (int step = 0; step < 20; ++step) {
            int a = int(rng() % n), b = int(rng() % n);
            if (rollback.merge(a, b))
                edges.push_back({a, b});
        }
        checkpoints.push_back({rollback.time(), int(edges.size())});
        if (block % 7 == 6) {
            int pick = int(rng() % checkpoints.size());
            rollback.rollback(checkpoints[pick].first);
            edges.resize(checkpoints[pick].second);
            checkpoints.resize(pick + 1);
        }

        ndsu reference_dsu(n);
        for (auto [a, b] : edges)
            reference_dsu.merge(a, b);
        for (int trial = 0; trial < 40; ++trial) {
            int a = int(rng() % n), b = int(rng() % n);
            ntest(rollback.same(a, b) == reference_dsu.same(a, b));
        }
    }
}
