#include "../include/hld.hpp"
#include "../include/segtree.hpp"
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x << '\n'; std::abort(); } } while (false)
struct Both { std::string forward, backward; };
struct Concat {
    using value_type = Both;
    Both id() const { return {}; }
    Both join(const Both& a, const Both& b) const { return {a.forward + b.forward, b.backward + a.backward}; }
};

int main() {
    std::mt19937 rng(7771);
    for (int trial = 0; trial < 1000; ++trial) {
        int n = 1 + int(rng() % 80), root = int(rng() % unsigned(n));
        std::vector<std::vector<int>> adj(n);
        for (int v = 1; v < n; ++v) {
            int p = int(rng() % unsigned(v));
            adj[v].push_back(p); adj[p].push_back(v);
        }
        std::vector<int> parent(n, -1), depth(n), queue{root};
        parent[root] = root;
        for (int at = 0; at < n; ++at)
            for (int u : adj[queue[at]]) if (parent[u] < 0) {
                parent[u] = queue[at]; depth[u] = depth[queue[at]] + 1; queue.push_back(u);
            }
        auto tree = [&] {
            auto temporary = adj;
            return hld(n, root, [&](int v) -> const auto& { return temporary[v]; });
        }();
        CHECK(tree.parent == parent && tree.depth == depth);
        std::vector<int> seen(n);
        for (int v = 0; v < n; ++v) {
            CHECK(tree.order[tree.pos[v]] == v);
            CHECK(++seen[tree.pos[v]] == 1);
            auto [left, right] = tree.subtree(v);
            for (int u = 0; u < n; ++u) {
                int ancestor = u;
                while (ancestor != v && ancestor != root) ancestor = parent[ancestor];
                CHECK((ancestor == v) == (left <= tree.pos[u] && tree.pos[u] < right));
            }
        }
        std::vector<Both> base(n);
        for (int v = 0; v < n; ++v) {
            std::string value(1, char('a' + v % 26));
            base[tree.pos[v]] = {value, value};
        }
        segtree<Concat> segment(base);
        for (int query = 0; query < 80; ++query) {
            int a = int(rng() % unsigned(n)), b = int(rng() % unsigned(n));
            int x = a, y = b;
            std::vector<int> prefix, suffix;
            while (x != y) {
                if (depth[x] >= depth[y]) { prefix.push_back(x); x = parent[x]; }
                else { suffix.push_back(y); y = parent[y]; }
            }
            CHECK(tree.lca(a, b) == x);
            prefix.push_back(x);
            while (!suffix.empty()) { prefix.push_back(suffix.back()); suffix.pop_back(); }
            for (bool edges : {false, true}) {
                std::vector<int> expected;
                for (int v : prefix) if (!edges || v != x) expected.push_back(v);
                std::vector<int> actual;
                std::string ordered;
                tree.path(a, b, [&](path_piece piece) {
                    auto value = segment.fold(piece.left, piece.right);
                    ordered += piece.reversed ? value.backward : value.forward;
                    for (int i = 0; i < piece.right - piece.left; ++i)
                        actual.push_back(tree.order[piece.reversed ? piece.right - 1 - i : piece.left + i]);
                }, edges);
                CHECK(actual == expected);
                std::string oracle;
                for (int v : expected) oracle += char('a' + v % 26);
                CHECK(ordered == oracle);
            }
        }
    }
    std::cout << "hld: 1000 random-root trees, 160000 ordered vertex/edge paths, subtree and lifetime checks\n";
}
