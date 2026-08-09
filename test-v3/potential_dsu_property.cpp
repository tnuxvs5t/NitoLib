#include "../src-v3/ds.hpp"

void check(bool condition, const char* message) {
    if (!condition) throw runtime_error(message);
}

struct xor_group {
    int id() const { return 0; }
    int operator()(int a, int b) const { return a ^ b; }
    int inverse(int value) const { return value; }
};

template <class T, class G>
void random_test(G group, uint64_t seed) {
    mt19937_64 random(seed);
    for (int trial = 0; trial < 500; ++trial) {
        int n = 1 + int(random() % 24);
        npotential_dsu<T, G> dsu(n, group);
        vector<vector<pair<int, T>>> graph(n);

        auto brute = [&](int source, int target) -> optional<T> {
            vector<unsigned char> seen(n);
            vector<T> value(n, group.id());
            vector<int> queue{source};
            seen[source] = true;
            for (int at = 0; at < int(queue.size()); ++at) {
                int from = queue[at];
                for (auto [to, delta] : graph[from]) if (!seen[to]) {
                    seen[to] = true;
                    value[to] = group(value[from], delta);
                    queue.push_back(to);
                }
            }
            if (!seen[target]) return nullopt;
            return group(group.inverse(value[source]), value[target]);
        };

        for (int step = 0; step < 600; ++step) {
            int a = int(random() % n), b = int(random() % n);
            if (random() % 3) {
                T delta = T(random() % 31);
                auto old = brute(a, b);
                bool expected = !old || *old == delta;
                check(dsu.merge(a, b, delta) == expected, "merge consistency");
                if (!old) {
                    graph[a].push_back({b, delta});
                    graph[b].push_back({a, group.inverse(delta)});
                }
            } else {
                auto expected = brute(a, b);
                auto actual = dsu.difference(a, b);
                check(bool(actual) == bool(expected), "difference presence");
                if (actual) check(*actual == *expected, "difference value");
                check(dsu.same(a, b) == bool(expected), "same");
            }
        }
    }
}

int main() {
    random_test<long long>(nsum_group<long long>{}, 0x51ed5eedULL);
    random_test<int>(xor_group{}, 0xa11ceULL);
}
