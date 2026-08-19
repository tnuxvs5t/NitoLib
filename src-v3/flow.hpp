#pragma once
#include "ds.hpp"

/* Residual edges are stored in xor-pairs; add returns the forward edge handle. */
template <class C>
struct ndinic {
    struct edge { nidx_t to, next; C capacity; };
    vector<nidx_t> head, level, current;
    vector<edge> edges;

    explicit ndinic(nidx_t n = 0) : head(n, -1), level(n), current(n) {}
    nidx_t len() const { return nidx_t(head.size()); }
    nidx_t add(nidx_t from, nidx_t to, C capacity) {
        nidx_t handle = nidx_t(edges.size());
        edges.push_back({to, head[from], capacity});
        head[from] = handle;
        edges.push_back({from, head[to], C{}});
        head[to] = handle + 1;
        return handle;
    }

    bool layer(nidx_t source, nidx_t sink) {
        fill(level.begin(), level.end(), -1);
        vector<nidx_t> queue{source};
        level[source] = 0;
        for (nidx_t at = 0; at < nidx_t(queue.size()); ++at) {
            nidx_t from = queue[at];
            for (nidx_t handle = head[from]; handle >= 0; handle = edges[handle].next) {
                nidx_t to = edges[handle].to;
                if (edges[handle].capacity > C{} && level[to] < 0)
                    level[to] = level[from] + 1, queue.push_back(to);
            }
        }
        return level[sink] >= 0;
    }

    C augment(nidx_t from, nidx_t sink, C pushed) {
        if (from == sink) return pushed;
        for (nidx_t& handle = current[from]; handle >= 0; handle = edges[handle].next) {
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

    C flow(nidx_t source, nidx_t sink, C limit = numeric_limits<C>::max()) {
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

    vector<unsigned char> cut(nidx_t source) const {
        vector<unsigned char> reachable(len());
        vector<nidx_t> stack{source};
        reachable[source] = true;
        while (!stack.empty()) {
            nidx_t from = stack.back();
            stack.pop_back();
            for (nidx_t handle = head[from]; handle >= 0; handle = edges[handle].next) {
                nidx_t to = edges[handle].to;
                if (edges[handle].capacity > C{} && !reachable[to])
                    reachable[to] = true, stack.push_back(to);
            }
        }
        return reachable;
    }
};

struct nmatching {
    vector<nidx_t> left, right;
    nidx_t size;
};

/* next(left_vertex) enumerates dense right positions and must be repeatable. */
template <class Next>
nmatching nhopcroft_karp(nidx_t left_size, nidx_t right_size, Next next) {
    vector<nidx_t> left(left_size, -1), right(right_size, -1), distance(left_size), queue;
    auto bfs = [&] {
        queue.clear();
        fill(distance.begin(), distance.end(), -1);
        for (nidx_t vertex = 0; vertex < left_size; ++vertex)
            if (left[vertex] < 0) distance[vertex] = 0, queue.push_back(vertex);
        for (nidx_t at = 0; at < nidx_t(queue.size()); ++at) {
            nidx_t from = queue[at];
            for (nidx_t to : invoke(next, from))
                if (right[to] >= 0 && distance[right[to]] < 0)
                    distance[right[to]] = distance[from] + 1, queue.push_back(right[to]);
        }
    };
    auto dfs = [&](auto&& self, nidx_t from) -> bool {
        for (nidx_t to : invoke(next, from)) {
            nidx_t other = right[to];
            if (other < 0 || (distance[other] == distance[from] + 1 && self(self, other))) {
                left[from] = to;
                right[to] = from;
                return true;
            }
        }
        distance[from] = -1;
        return false;
    };
    nidx_t size = 0;
    while (true) {
        bfs();
        nidx_t added = 0;
        for (nidx_t vertex = 0; vertex < left_size; ++vertex)
            if (left[vertex] < 0) added += dfs(dfs, vertex);
        if (!added) break;
        size += added;
    }
    return {move(left), move(right), size};
}

template <class W>
struct nmst_result {
    W weight;
    vector<nidx_t> edges;
};

/* Edge projections receive edges[position]; the result is a minimum spanning forest. */
template <class V, class From, class To, class Weight>
auto nkruskal(nidx_t vertices, V edges, From from, To to, Weight weight) {
    using W = remove_cvref_t<decltype(invoke(weight, edges[0]))>;
    vector<nidx_t> order(edges.len());
    iota(order.begin(), order.end(), 0);
    sort(order.begin(), order.end(), [&](nidx_t a, nidx_t b) {
        return invoke(weight, edges[a]) < invoke(weight, edges[b]);
    });
    ndsu components(vertices);
    nmst_result<W> result{W{}, {}};
    for (nidx_t position : order) {
        auto&& edge = edges[position];
        nidx_t a = invoke(from, edge), b = invoke(to, edge);
        if (components.same(a, b)) continue;
        components.merge(a, b);
        result.weight += invoke(weight, edge);
        result.edges.push_back(position);
    }
    return result;
}
