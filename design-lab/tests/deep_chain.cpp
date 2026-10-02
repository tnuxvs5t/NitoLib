#include "../include/graph.hpp"
#include "../include/hld.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    constexpr int n = 300000;
    auto next = [](int v) {
        std::vector<int> out;
        if (v) out.push_back(v - 1);
        if (v + 1 < n) out.push_back(v + 1);
        return out;
    };
    hld tree(n, 0, next);
    auto distances = bfs(n, 0, next);
    int count = 0;
    tree.path(n - 1, 0, [&](path_piece piece) { count += piece.right - piece.left; });
    if (tree.size[0] != n || tree.depth.back() != n - 1 || distances.distance.back() != n - 1 || count != n) std::abort();
    std::cout << "deep chain: 300000 vertices, HLD+BFS, bounded process stack\n";
}
