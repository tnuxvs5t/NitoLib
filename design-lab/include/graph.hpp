#pragma once
#include <algorithm>
#include <functional>
#include <queue>
#include <utility>
#include <vector>

template <class Distance>
struct paths {
    std::vector<Distance> distance;
    std::vector<int> parent; // root points to itself; unreachable is -1
};

// Vertices are [0,n), source is valid. next(v) yields edges, to(edge) an int.
// Adjacency is borrowed only during this call. No graph descriptor is retained.
// O(n+m) plus next/to costs; queue and result use O(n) storage.
template <class Next, class To = std::identity>
paths<int> bfs(int n, int source, Next next, To to = {}) {
    paths<int> out{std::vector<int>(n, -1), std::vector<int>(n, -1)};
    std::vector<int> queue{source};
    queue.reserve(n);
    out.distance[source] = 0;
    out.parent[source] = source;
    for (int at = 0; at < int(queue.size()); ++at) {
        int v = queue[at];
        for (auto&& edge : std::invoke(next, v)) {
            int u = std::invoke(to, edge);
            if (out.distance[u] >= 0) continue;
            out.distance[u] = out.distance[v] + 1;
            out.parent[u] = v;
            queue.push_back(u);
        }
    }
    return out;
}

// Weights are nonnegative. Distance is an ordered numeric type. infinity is a
// positive sentinel; only paths strictly below it are represented. Candidates at
// or above it are skipped before addition, avoiding signed-integer overflow.
// O((n+m) log(m+2)) plus port costs; heap O(m), result O(n).
template <class Distance, class Next, class To, class Cost>
paths<Distance> dijkstra(int n, int source, Next next, To to, Cost cost,
                         Distance infinity) {
    paths<Distance> out{std::vector<Distance>(n, infinity), std::vector<int>(n, -1)};
    using Item = std::pair<Distance, int>;
    std::priority_queue<Item, std::vector<Item>, std::greater<>> queue;
    out.distance[source] = Distance{};
    out.parent[source] = source;
    queue.emplace(Distance{}, source);
    while (!queue.empty()) {
        auto [distance, v] = queue.top();
        queue.pop();
        if (distance != out.distance[v]) continue;
        for (auto&& edge : std::invoke(next, v)) {
            int u = std::invoke(to, edge);
            Distance weight = std::invoke(cost, edge);
            if (weight >= infinity - distance) continue;
            Distance candidate = distance + weight;
            if (candidate >= out.distance[u]) continue;
            out.distance[u] = candidate;
            out.parent[u] = v;
            queue.emplace(candidate, u);
        }
    }
    return out;
}

// parent is a predecessor forest as produced above; target is in range.
// Empty for unreachable, otherwise source-to-target vertices. O(path length).
inline std::vector<int> restore_path(const std::vector<int>& parent, int target) {
    if (parent[target] < 0) return {};
    std::vector<int> path{target};
    while (parent[target] != target) {
        target = parent[target];
        path.push_back(target);
    }
    std::reverse(path.begin(), path.end());
    return path;
}
