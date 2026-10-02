#pragma once
#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

struct path_piece { int left, right; bool reversed; };

// One connected undirected tree, vertices [0,n), n>0; next(v) yields adjacent int
// vertices in a stable order. Construction uses O(n) heap storage, O(1) call depth.
// Owns all metadata and retains no adjacency/owner references. No hidden rooting
// descriptor, partial forest semantics, or duplicated key/position interface.
struct hld {
    std::vector<int> parent, depth, size, heavy, head, pos, order;

    template <class Next>
    hld(int n, int root, Next next)
        : parent(n, -1), depth(n), size(n, 1), heavy(n, -1),
          head(n), pos(n), order(n) {
        std::vector<int> traversal{root};
        traversal.reserve(n);
        parent[root] = root;
        for (int at = 0; at < int(traversal.size()); ++at) {
            int v = traversal[at];
            for (int u : std::invoke(next, v)) {
                if (u == parent[v]) continue;
                parent[u] = v;
                depth[u] = depth[v] + 1;
                traversal.push_back(u);
            }
        }
        for (int at = n - 1; at > 0; --at) {
            int v = traversal[at], p = parent[v];
            size[p] += size[v];
            if (heavy[p] < 0 || size[v] > size[heavy[p]]) heavy[p] = v;
        }
        std::vector<std::pair<int, int>> pending{{root, root}};
        int timer = 0;
        while (!pending.empty()) {
            auto [first, top] = pending.back();
            pending.pop_back();
            for (int v = first; v >= 0; v = heavy[v]) {
                head[v] = top;
                pos[v] = timer;
                order[timer++] = v;
                for (int u : std::invoke(next, v))
                    if (parent[u] == v && u != v && u != heavy[v]) pending.emplace_back(u, u);
            }
        }
    }

    int lca(int a, int b) const {
        while (head[a] != head[b]) {
            if (depth[head[a]] < depth[head[b]]) std::swap(a, b);
            a = parent[head[a]];
        }
        return depth[a] < depth[b] ? a : b;
    }

    std::pair<int, int> subtree(int v) const { return {pos[v], pos[v] + size[v]}; }

    // Emits ordered [left,right) pieces from a to b, O(log n), no heap allocation.
    // reversed means read this base interval right-to-left. Edge mode associates
    // each edge with its deeper vertex and excludes exactly the LCA's position.
    template <class Visit>
    void path(int a, int b, Visit visit, bool edges = false) const {
        std::array<path_piece, std::numeric_limits<int>::digits + 1> suffix;
        int count = 0;
        while (head[a] != head[b]) {
            if (depth[head[a]] >= depth[head[b]]) {
                std::invoke(visit, path_piece{pos[head[a]], pos[a] + 1, true});
                a = parent[head[a]];
            } else {
                suffix[count++] = {pos[head[b]], pos[b] + 1, false};
                b = parent[head[b]];
            }
        }
        int left = std::min(pos[a], pos[b]) + int(edges);
        int right = std::max(pos[a], pos[b]) + 1;
        if (left < right) {
            if (depth[a] >= depth[b]) std::invoke(visit, path_piece{left, right, true});
            else suffix[count++] = {left, right, false};
        }
        while (count) std::invoke(visit, suffix[--count]);
    }
};
