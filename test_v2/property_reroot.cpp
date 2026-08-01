#include "common.hpp"

struct nproperty_distance {
    long long vertices, sum;
};

int main() {
    mt19937 rng(26457513);
    for (int trial = 0; trial < 500; ++trial) {
        int n = 1 + int(rng() % 100);
        ngraph_list<int> tree(n);
        for (int vertex = 1; vertex < n; ++vertex)
            tree.add2(vertex, int(rng() % vertex));

        auto result = nreroot(
            tree, nproperty_distance{0, 0},
            [](nproperty_distance a, nproperty_distance b) {
                return nproperty_distance{a.vertices + b.vertices, a.sum + b.sum};
            },
            [](nproperty_distance value, int) {
                ++value.vertices;
                return value;
            },
            [](nproperty_distance value, int, int) {
                value.sum += value.vertices;
                return value;
            });

        for (int source = 0; source < n; ++source) {
            auto distance = nbfs(tree, source);
            long long expected = 0;
            for (int vertex = 0; vertex < n; ++vertex)
                expected += distance[vertex];
            ntest(result[source].vertices == n);
            ntest(result[source].sum == expected);
        }
    }
}
