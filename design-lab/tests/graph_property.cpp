#include "../include/graph.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x << '\n'; std::abort(); } } while (false)
struct Edge { int to; long long cost; };

int main() {
    constexpr long long inf = 1LL << 60;
    std::mt19937 rng(9204);
    for (int trial = 0; trial < 900; ++trial) {
        int n = 1 + int(rng() % 24), source = int(rng() % unsigned(n));
        std::vector<std::vector<Edge>> adj(n);
        std::vector<std::vector<long long>> weighted(n, std::vector<long long>(n, inf));
        std::vector<std::vector<int>> hops(n, std::vector<int>(n, 1000000));
        for (int v = 0; v < n; ++v) weighted[v][v] = hops[v][v] = 0;
        for (int i = 0; i < n * n; ++i) if (rng() % 4 == 0) {
            int a = int(rng() % unsigned(n)), b = int(rng() % unsigned(n));
            long long w = rng() % 20;
            adj[a].push_back({b, w});
            weighted[a][b] = std::min(weighted[a][b], w);
            hops[a][b] = std::min(hops[a][b], 1);
        }
        for (int k = 0; k < n; ++k)
            for (int i = 0; i < n; ++i)
                for (int j = 0; j < n; ++j) {
                    weighted[i][j] = std::min(weighted[i][j], weighted[i][k] + weighted[k][j]);
                    hops[i][j] = std::min(hops[i][j], hops[i][k] + hops[k][j]);
                }
        auto next = [&](int v) -> const auto& { return adj[v]; };
        auto shortest = dijkstra(n, source, next, &Edge::to, &Edge::cost, inf);
        auto breadth = bfs(n, source, next, &Edge::to);
        CHECK(shortest.distance == weighted[source]);
        for (int v = 0; v < n; ++v) {
            CHECK(breadth.distance[v] == (hops[source][v] == 1000000 ? -1 : hops[source][v]));
            auto path = restore_path(shortest.parent, v);
            if (weighted[source][v] == inf) { CHECK(path.empty()); continue; }
            CHECK(path.front() == source && path.back() == v && path.size() <= unsigned(n));
            for (int i = 1; i < int(path.size()); ++i) {
                bool valid = false;
                for (auto edge : adj[path[i - 1]])
                    if (edge.to == path[i] && shortest.distance[path[i - 1]] + edge.cost == shortest.distance[path[i]]) valid = true;
                CHECK(valid);
            }
        }
    }
    // Saturating at infinity must avoid evaluating an overflowing sum.
    long long maximum = std::numeric_limits<long long>::max();
    std::vector<std::vector<Edge>> huge{{{1, maximum - 1}}, {{2, maximum}}, {}};
    auto result = dijkstra(3, 0, [&](int v) -> const auto& { return huge[v]; },
                                  &Edge::to, &Edge::cost, maximum);
    CHECK(result.distance[1] == maximum - 1 && result.parent[2] == -1);
    // Implicit adjacency owns each temporary only for its range-for duration.
    auto grid = bfs(35, 0, [](int v) {
        std::vector<int> out;
        int x = v / 7, y = v % 7;
        if (x) out.push_back(v - 7);
        if (x < 4) out.push_back(v + 7);
        if (y) out.push_back(v - 1);
        if (y < 6) out.push_back(v + 1);
        return out;
    });
    for (int v = 0; v < 35; ++v) CHECK(grid.distance[v] == v / 7 + v % 7);
    std::cout << "graph: 900 Floyd-Warshall oracles + zero cycles, paths, overflow, implicit grid\n";
}
