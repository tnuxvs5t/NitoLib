#include "../include/graph.hpp"
#include <iostream>

int main() {
    struct Edge { int to; long long cost; };
    std::vector<std::vector<Edge>> adj{
        {{1, 4}, {2, 1}}, {{3, 2}}, {{1, 1}, {3, 8}}, {}
    };
    auto result = dijkstra(4, 0,
        [&](int v) -> const auto& { return adj[v]; },
        &Edge::to, &Edge::cost, 1LL << 60);
    std::cout << "distance:";
    for (auto x : result.distance) std::cout << ' ' << x;
    std::cout << "\npath:";
    for (int v : restore_path(result.parent, 3)) std::cout << ' ' << v;
    std::cout << '\n';
}
