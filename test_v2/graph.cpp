#include "common.hpp"

int main() {
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
}
