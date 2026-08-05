#include "common.hpp"

template <class E> struct nnoncopy_adjacency {
    nvector<E> edges;
    nnoncopy_adjacency() = default;
    nnoncopy_adjacency(const nnoncopy_adjacency&) = delete;
    nnoncopy_adjacency& operator=(const nnoncopy_adjacency&) = delete;
    int len() const { return edges.len(); }
    const E& operator[](int index) const { return edges[index]; }
};

template <int Vertices> struct nreference_graph {
    array<nnoncopy_adjacency<narc<int>>, Vertices> adjacency;
    int vertices() const { return int(adjacency.size()); }
    const auto& neighbors(int vertex) const { return adjacency[vertex]; }
    void add(int from, int to, int weight = 1) { adjacency[from].edges.push(narc<int>{to, weight}); }
    void add2(int a, int b, int weight = 1) {
        add(a, b, weight);
        add(b, a, weight);
    }
};

struct ninvalid_adjacency_graph {
    int vertices() const { return 1; }
    int neighbors(int) const { return 0; }
};

int main() {
    static_assert(!ngraph_like<ninvalid_adjacency_graph>);

    constexpr int height = 5, width = 7;
    auto grid = ngraph_view(height * width, [=](int vertex) {
        int row = vertex / width, column = vertex % width;
        array<int, 4> next{};
        int degree = 0;
        if (row)
            next[degree++] = vertex - width;
        if (row + 1 < height)
            next[degree++] = vertex + width;
        if (column)
            next[degree++] = vertex - 1;
        if (column + 1 < width)
            next[degree++] = vertex + 1;
        return nview(degree, [next](int i) { return next[i]; });
    });

    static_assert(ngraph_like<decltype(grid)>);
    auto distance = nbfs(grid, 0);
    for (int row = 0; row < height; ++row)
        for (int column = 0; column < width; ++column)
            ntest(distance[row * width + column] == row + column);

    ngraph_list<int> graph(6);
    graph.add2(0, 1, 7);
    graph.add2(0, 2, 2);
    graph.add2(2, 1, 1);
    graph.add2(1, 3, 3);
    graph.add2(2, 4, 8);
    graph.add2(3, 5, 4);
    graph.add2(4, 5, 1);
    auto shortest = ndijkstra(graph, 0);
    ntest((shortest == nvector<long long>{0, 3, 2, 6, 10, 10}));

    auto implicit_weighted = ngraph_view(5, [](int vertex) {
        return nview(vertex + 1 < 5 ? 1 : 0, [vertex](int) { return narc<int>{vertex + 1, vertex + 2}; });
    });
    auto path = ndijkstra(implicit_weighted, 0);
    ntest((path == nvector<long long>{0, 2, 5, 9, 14}));

    ngraph_list<int> large_int_distance(3);
    large_int_distance.add(0, 1, 400'000'000);
    large_int_distance.add(1, 2, 400'000'000);
    ntest((ndijkstra<int>(large_int_distance, 0) == nvector<int>{0, 400'000'000, 800'000'000}));

    nreference_graph<4> borrowed;
    borrowed.add(0, 1, 4);
    borrowed.add(0, 2, 1);
    borrowed.add(2, 1, 1);
    borrowed.add(1, 3, 2);
    ntest((nbfs(borrowed, 0) == nvector<int>{0, 1, 1, 2}));
    ntest((ndijkstra(borrowed, 0) == nvector<long long>{0, 2, 1, 4}));

    auto order = ntoposort(borrowed);
    ntest(order && order->len() == borrowed.vertices());
    ntest(nscc(borrowed).classes() == borrowed.vertices());

    nreference_graph<4> borrowed_tree;
    borrowed_tree.add2(0, 1);
    borrowed_tree.add2(1, 2);
    borrowed_tree.add2(1, 3);
    nlca borrowed_lca(borrowed_tree);
    ntest(borrowed_lca(2, 3) == 1);
    auto rooted_sizes = nreroot(
        borrowed_tree, 0, [](int a, int b) { return a + b; },
        [](int aggregate, int) { return aggregate + 1; },
        [](int state, int, int) { return state; });
    ntest((rooted_sizes == nvector<int>{4, 4, 4, 4}));

    nreference_graph<3> binary_weighted;
    binary_weighted.add(0, 1, 1);
    binary_weighted.add(0, 2, 0);
    binary_weighted.add(2, 1, 0);
    ntest((n01bfs(binary_weighted, 0) == nvector<int>{0, 0, 0}));

    nreference_graph<3> undirected_weighted;
    undirected_weighted.add2(0, 1, 4);
    undirected_weighted.add2(0, 2, 1);
    undirected_weighted.add2(2, 1, 1);
    auto spanning = nprim<long long>(undirected_weighted);
    ntest(spanning && spanning->weight == 2);

    nreference_graph<3> bipartite;
    bipartite.add(0, 0);
    bipartite.add(0, 1);
    bipartite.add(1, 1);
    bipartite.add(2, 2);
    ntest(nhopcroft_karp(bipartite, 3).size == 3);
}
