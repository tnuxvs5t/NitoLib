#pragma once
#include "ds.hpp"

/* Residual edges are stored in xor-pairs; add returns the forward edge handle. */
template <class C>
struct ndinic {
    struct edge { int to, next; C capacity; };
    vector<int> head, level, current;
    vector<edge> edges;

    explicit ndinic(int n = 0) : head(n, -1), level(n), current(n) {}
    int len() const { return int(head.size()); }
    int add(int from, int to, C capacity) {
        int handle = int(edges.size());
        edges.push_back({to, head[from], capacity});
        head[from] = handle;
        edges.push_back({from, head[to], C{}});
        head[to] = handle + 1;
        return handle;
    }

    bool layer(int source, int sink) {
        fill(level.begin(), level.end(), -1);
        vector<int> queue{source};
        level[source] = 0;
        for (int at = 0; at < int(queue.size()); ++at) {
            int from = queue[at];
            for (int handle = head[from]; handle >= 0; handle = edges[handle].next) {
                int to = edges[handle].to;
                if (edges[handle].capacity > C{} && level[to] < 0)
                    level[to] = level[from] + 1, queue.push_back(to);
            }
        }
        return level[sink] >= 0;
    }

    C augment(int from, int sink, C pushed) {
        if (from == sink) return pushed;
        for (int& handle = current[from]; handle >= 0; handle = edges[handle].next) {
            edge& item = edges[handle];
            if (!(item.capacity > C{}) || level[item.to] != level[from] + 1) continue;
            C sent = augment(item.to, sink, min(pushed, item.capacity));
            if (sent > C{}) {
                item.capacity -= sent;
                edges[handle ^ 1].capacity += sent;
                return sent;
            }
        }
        return C{};
    }

    C flow(int source, int sink, C limit = numeric_limits<C>::max()) {
        C result{};
        while (result < limit && layer(source, sink)) {
            current = head;
            while (result < limit) {
                C sent = augment(source, sink, limit - result);
                if (!(sent > C{})) break;
                result += sent;
            }
        }
        return result;
    }

    vector<unsigned char> cut(int source) const {
        vector<unsigned char> reachable(len());
        vector<int> stack{source};
        reachable[source] = true;
        while (!stack.empty()) {
            int from = stack.back();
            stack.pop_back();
            for (int handle = head[from]; handle >= 0; handle = edges[handle].next) {
                int to = edges[handle].to;
                if (edges[handle].capacity > C{} && !reachable[to])
                    reachable[to] = true, stack.push_back(to);
            }
        }
        return reachable;
    }
};

struct nmatching {
    vector<int> left, right;
    int size;
};

/* next(left_vertex) enumerates dense right positions and must be repeatable. */
template <class Next>
nmatching nhopcroft_karp(int left_size, int right_size, Next next) {
    vector<int> left(left_size, -1), right(right_size, -1), distance(left_size), queue;
    auto bfs = [&] {
        queue.clear();
        fill(distance.begin(), distance.end(), -1);
        for (int vertex = 0; vertex < left_size; ++vertex)
            if (left[vertex] < 0) distance[vertex] = 0, queue.push_back(vertex);
        for (int at = 0; at < int(queue.size()); ++at) {
            int from = queue[at];
            for (int to : invoke(next, from))
                if (right[to] >= 0 && distance[right[to]] < 0)
                    distance[right[to]] = distance[from] + 1, queue.push_back(right[to]);
        }
    };
    auto dfs = [&](auto&& self, int from) -> bool {
        for (int to : invoke(next, from)) {
            int other = right[to];
            if (other < 0 || (distance[other] == distance[from] + 1 && self(self, other))) {
                left[from] = to;
                right[to] = from;
                return true;
            }
        }
        distance[from] = -1;
        return false;
    };
    int size = 0;
    while (true) {
        bfs();
        int added = 0;
        for (int vertex = 0; vertex < left_size; ++vertex)
            if (left[vertex] < 0) added += dfs(dfs, vertex);
        if (!added) break;
        size += added;
    }
    return {move(left), move(right), size};
}

template <class W>
struct nmst_result {
    W weight;
    vector<int> edges;
};

/* Edge projections receive edges[position]; the result is a minimum spanning forest. */
template <class V, class From, class To, class Weight>
auto nkruskal(int vertices, V edges, From from, To to, Weight weight) {
    using W = remove_cvref_t<decltype(invoke(weight, edges[0]))>;
    vector<int> order(edges.len());
    iota(order.begin(), order.end(), 0);
    sort(order.begin(), order.end(), [&](int a, int b) {
        return invoke(weight, edges[a]) < invoke(weight, edges[b]);
    });
    ndsu components(vertices);
    nmst_result<W> result{W{}, {}};
    for (int position : order) {
        auto&& edge = edges[position];
        int a = invoke(from, edge), b = invoke(to, edge);
        if (components.same(a, b)) continue;
        components.merge(a, b);
        result.weight += invoke(weight, edge);
        result.edges.push_back(position);
    }
    return result;
}
