#include "common.hpp"

static nvector<int> brute_path(int from, int to, const nvector<int>& parent,
                               const nvector<int>& depth) {
    nvector<int> left, right;
    while (from != to) {
        if (depth[from] >= depth[to]) {
            left.push(from);
            from = parent[from];
        } else {
            right.push(to);
            to = parent[to];
        }
    }
    left.push(from);
    while (!right.empty())
        left.push(right.pop());
    return left;
}

int main() {
    mt19937 random(0x42c0ffeeU);
    for (int repeat = 0; repeat < 240; ++repeat) {
        int n = 1 + int(random() % 70);
        ngraph_forward<int> graph(n, max(0, 2 * n - 2));
        nvector<int> parent(n, 0), depth(n, 0), edge_weight(n, 0);
        nvector<long long> root_distance(n, 0);
        for (int vertex = 1; vertex < n; ++vertex) {
            parent[vertex] = int(random() % vertex);
            depth[vertex] = depth[parent[vertex]] + 1;
            edge_weight[vertex] = 1 + int(random() % 100);
            root_distance[vertex] = root_distance[parent[vertex]] + edge_weight[vertex];
            graph.add2(parent[vertex], vertex, edge_weight[vertex]);
        }

        nhld decomposition(graph);
        nlca_binary<long long> lca(graph);
        for (int attempt = 0; attempt < 240; ++attempt) {
            int from = int(random() % n), to = int(random() % n);
            auto expected = brute_path(from, to, parent, depth);
            nvector<int> actual;
            nfor(segment, decomposition.path(from, to)) {
                if (segment.rev) {
                    for (int position = segment.r; position-- > segment.l;)
                        actual.push(decomposition.vertex(position));
                } else {
                    for (int position = segment.l; position < segment.r; ++position)
                        actual.push(decomposition.vertex(position));
                }
            }
            ntest(actual == expected);

            int common = expected[0];
            nfor(vertex, expected)
                if (depth[vertex] < depth[common])
                    common = vertex;
            ntest(decomposition.lca(from, to) == common && lca.lca(from, to) == common);
            ntest(lca.dist(from, to) == root_distance[from] + root_distance[to] -
                                              2 * root_distance[common]);
            for (int index = 0; index < expected.len(); ++index)
                ntest(lca.kth(from, to, index) == expected[index]);

            auto edges = decomposition.path(from, to, true);
            int edge_count = 0;
            nfor(segment, edges)
                edge_count += segment.r - segment.l;
            ntest(edge_count + 1 == expected.len());
        }

        for (int vertex = 0; vertex < n; ++vertex) {
            auto [left, right] = decomposition.subtree(vertex);
            int expected_size = 0;
            for (int candidate = 0; candidate < n; ++candidate) {
                int ancestor = candidate;
                while (depth[ancestor] > depth[vertex])
                    ancestor = parent[ancestor];
                bool inside = ancestor == vertex;
                expected_size += inside;
                bool in_interval = left <= decomposition.position(candidate) &&
                                   decomposition.position(candidate) < right;
                ntest(inside == in_interval);
            }
            ntest(right - left == expected_size);
        }
    }
}
