#include "common.hpp"

int main() {
    mt19937 rng(24494897);

    for (int trial = 0; trial < 200; ++trial) {
        int n = 1 + int(rng() % 60);
        ngraph_list<int> dag(n);
        for (int from = 0; from < n; ++from)
            for (int to = from + 1; to < n; ++to)
                if (rng() % 10 == 0)
                    dag.add(from, to);
        auto order = ntoposort(dag);
        ntest(order && order->len() == n);
        nvector<int> position(n);
        for (int i = 0; i < n; ++i)
            position[(*order)[i]] = i;
        for (int from = 0; from < n; ++from) {
            auto adjacency = dag.neighbors(from);
            nfor(edge, adjacency) ntest(position[from] < position[edge.to]);
        }
    }

    for (int trial = 0; trial < 100; ++trial) {
        int n = 1 + int(rng() % 35);
        ngraph_list<int> graph(n);
        vector<vector<bool>> reachable(n, vector<bool>(n));
        for (int vertex = 0; vertex < n; ++vertex)
            reachable[vertex][vertex] = true;
        for (int from = 0; from < n; ++from)
            for (int to = 0; to < n; ++to)
                if (rng() % 12 == 0) {
                    graph.add(from, to);
                    reachable[from][to] = true;
                }
        for (int middle = 0; middle < n; ++middle)
            for (int from = 0; from < n; ++from)
                for (int to = 0; to < n; ++to)
                    reachable[from][to] = reachable[from][to] ||
                                          (reachable[from][middle] && reachable[middle][to]);
        auto component = nscc(graph);
        for (int a = 0; a < n; ++a)
            for (int b = 0; b < n; ++b)
                ntest(component.same(a, b) == (reachable[a][b] && reachable[b][a]));
    }

    for (int trial = 0; trial < 200; ++trial) {
        int n = 1 + int(rng() % 100);
        ngraph_list<int> tree(n);
        vector<int> parent(n, 0), depth(n, 0);
        for (int vertex = 1; vertex < n; ++vertex) {
            parent[vertex] = int(rng() % vertex);
            depth[vertex] = depth[parent[vertex]] + 1;
            tree.add2(vertex, parent[vertex]);
        }
        nlca lca(tree);
        for (int query = 0; query < 500; ++query) {
            int a = int(rng() % n), b = int(rng() % n);
            int x = a, y = b;
            while (depth[x] > depth[y])
                x = parent[x];
            while (depth[y] > depth[x])
                y = parent[y];
            while (x != y)
                x = parent[x], y = parent[y];
            ntest(lca(a, b) == x);
            ntest(lca.distance(a, b) == depth[a] + depth[b] - 2 * depth[x]);
        }
    }
}
