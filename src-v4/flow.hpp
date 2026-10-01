#pragma once
#include "ds.hpp"

template <class C>
struct ndinic {
    struct edge { nidx_t to, next; C capacity; };

    vector<nidx_t> head, level, current;
    vector<edge> edges;

    explicit ndinic(nidx_t n = 0) : head(n, -1), level(n), current(n) {}

    nidx_t len() const { return nidx_t(head.size()); }

    nidx_t add(nidx_t from, nidx_t to, C capacity) {
        nidx_t handle = nidx_t(edges.size());
        edges.push_back({to, head[from], move(capacity)});
        head[from] = handle;
        edges.push_back({from, head[to], C{}});
        head[to] = handle + 1;
        return handle;
    }

    bool layer(nidx_t source, nidx_t sink) {
        ranges::fill(level, -1);
        vector<nidx_t> queue{source};
        level[source] = 0;
        for (nidx_t at = 0; at < nidx_t(queue.size()); ++at) {
            nidx_t from = queue[at];
            for (nidx_t handle = head[from]; handle >= 0;
                 handle = edges[handle].next) {
                edge& item = edges[handle];
                if (item.capacity > C{} && level[item.to] < 0)
                    level[item.to] = level[from] + 1, queue.push_back(item.to);
            }
        }
        return level[sink] >= 0;
    }

    C augment(nidx_t from, nidx_t sink, C pushed) {
        if (from == sink) return pushed;
        for (nidx_t& handle = current[from]; handle >= 0;
             handle = edges[handle].next) {
            edge& item = edges[handle];
            if (!(item.capacity > C{}) || level[item.to] != level[from] + 1)
                continue;
            C sent = augment(item.to, sink, min(pushed, item.capacity));
            if (sent > C{}) {
                item.capacity -= sent;
                edges[handle ^ 1].capacity += sent;
                return sent;
            }
        }
        return C{};
    }

    C flow(nidx_t source, nidx_t sink,
           C limit = numeric_limits<C>::max()) {
        if (source == sink) return C{};
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
            for (nidx_t handle = head[from]; handle >= 0;
                 handle = edges[handle].next) {
                const edge& item = edges[handle];
                if (item.capacity > C{} && !reachable[item.to])
                    reachable[item.to] = true, stack.push_back(item.to);
            }
        }
        return reachable;
    }
};

struct nmatching {
    vector<nidx_t> left, right;
    nidx_t size;
};

/* next(left) is read once; each yielded right position must be in [0,right_size). */
template <class Next>
nmatching nhopcroft_karp(nidx_t left_size, nidx_t right_size, Next next) {
    vector<vector<nidx_t>> adj(left_size);
    for (nidx_t left = 0; left < left_size; ++left) {
        auto&& choices = invoke(next, left);
        for (auto&& right : choices) adj[left].push_back(nidx_t(right));
    }

    vector<nidx_t> left(left_size, -1), right(right_size, -1);
    vector<nidx_t> dist(left_size), at(left_size), queue;
    nidx_t shortest = -1;

    auto bfs = [&] {
        ranges::fill(dist, -1);
        queue.clear();
        shortest = -1;
        for (nidx_t vertex = 0; vertex < left_size; ++vertex)
            if (left[vertex] < 0)
                dist[vertex] = 0, queue.push_back(vertex);
        for (nidx_t head = 0; head < nidx_t(queue.size()); ++head) {
            nidx_t from = queue[head];
            if (shortest >= 0 && dist[from] >= shortest) continue;
            for (nidx_t to : adj[from]) {
                nidx_t other = right[to];
                if (other < 0) {
                    shortest = dist[from] + 1;
                } else if (dist[other] < 0) {
                    dist[other] = dist[from] + 1;
                    queue.push_back(other);
                }
            }
        }
        return shortest >= 0;
    };

    auto dfs = [&](auto&& self, nidx_t from) -> bool {
        for (nidx_t& i = at[from]; i < nidx_t(adj[from].size()); ++i) {
            nidx_t to = adj[from][i], other = right[to];
            if (other < 0) {
                if (dist[from] + 1 != shortest) continue;
            } else if (dist[other] != dist[from] + 1 ||
                       !self(self, other)) {
                continue;
            }
            left[from] = to;
            right[to] = from;
            return true;
        }
        dist[from] = -1;
        return false;
    };

    nidx_t size = 0;
    while (bfs()) {
        ranges::fill(at, 0);
        for (nidx_t vertex = 0; vertex < left_size; ++vertex)
            if (left[vertex] < 0 && dfs(dfs, vertex)) ++size;
    }
    return {move(left), move(right), size};
}

template <class W>
struct nmst_result {
    W weight;
    vector<nidx_t> edges;
};

/* Returns a minimum spanning forest; edges stores positions in the input range. */
template <class V, class From, class To, class Weight>
auto nkruskal(nidx_t vertices, V&& edges, From from, To to, Weight weight) {
    using W = remove_cvref_t<invoke_result_t<Weight&, decltype(edges[0])>>;
    vector<nidx_t> order(nlen(edges));
    iota(order.begin(), order.end(), 0);
    ranges::sort(order, [&](nidx_t left, nidx_t right) {
        return invoke(weight, edges[left]) < invoke(weight, edges[right]);
    });

    ndsu components(vertices);
    nmst_result<W> result{W{}, {}};
    for (nidx_t position : order) {
        auto&& edge = edges[position];
        nidx_t a = nidx_t(invoke(from, edge));
        nidx_t b = nidx_t(invoke(to, edge));
        if (components.same(a, b)) continue;
        components.merge(a, b);
        result.weight += invoke(weight, edge);
        result.edges.push_back(position);
    }
    return result;
}
