#include "common.hpp"

int main() {
    mt19937 rng(28284271);

    for (int trial = 0; trial < 300; ++trial) {
        int n = 1 + int(rng() % 50);
        struct edge_data {
            int a, b, weight;
        };
        vector<edge_data> edges;
        ngraph_list<int> graph(n);
        for (int vertex = 1; vertex < n; ++vertex) {
            int parent = int(rng() % vertex), weight = int(rng() % 101) - 50;
            edges.push_back({vertex, parent, weight});
            graph.add2(vertex, parent, weight);
        }
        for (int extra = 0; extra < n * 2; ++extra) {
            int a = int(rng() % n), b = int(rng() % n);
            if (a == b)
                continue;
            int weight = int(rng() % 101) - 50;
            edges.push_back({a, b, weight});
            graph.add2(a, b, weight);
        }
        sort(edges.begin(), edges.end(), [](const edge_data& a, const edge_data& b) { return a.weight < b.weight; });
        ndsu dsu(n);
        long long expected = 0;
        int used = 0;
        for (auto edge : edges)
            if (!dsu.same(edge.a, edge.b)) {
                dsu.merge(edge.a, edge.b);
                expected += edge.weight;
                ++used;
            }
        auto actual = nprim(graph);
        ntest(actual && actual->weight == expected && actual->edges.len() == used);
    }

    for (int trial = 0; trial < 1000; ++trial) {
        int n = 2 + int(rng() % 7), source = 0, sink = n - 1;
        struct input_edge {
            int from, to, capacity;
        };
        vector<input_edge> edges;
        nmaxflow<long long> flow(n);
        for (int from = 0; from < n; ++from)
            for (int to = 0; to < n; ++to)
                if (from != to && rng() % 5 == 0) {
                    int capacity = int(rng() % 11);
                    edges.push_back({from, to, capacity});
                    flow.add(from, to, capacity);
                }

        long long expected = numeric_limits<long long>::max();
        for (int mask = 0; mask < (1 << n); ++mask) {
            if (!(mask & (1 << source)) || (mask & (1 << sink)))
                continue;
            long long cut = 0;
            for (auto edge : edges)
                if ((mask & (1 << edge.from)) && !(mask & (1 << edge.to)))
                    cut += edge.capacity;
            expected = min(expected, cut);
        }
        ntest(flow.flow(source, sink) == expected);
    }
}
