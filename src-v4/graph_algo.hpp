#pragma once
#include "graph.hpp"

template <class W>
struct ndenseprim_result {
    W weight;
    vector<nidx_t> parent;
    nidx_t components;
};

template <class Weight, class W>
ndenseprim_result<W> ndenseprim(nidx_t n, Weight&& weight, W infinity) {
    ndenseprim_result<W> result{W{}, vector<nidx_t>(n, -1), 0};
    vector<W> best(n, infinity);
    vector<unsigned char> used(n);
    for (nidx_t step = 0; step < n; ++step) {
        nidx_t from = -1;
        for (nidx_t v = 0; v < n; ++v)
            if (!used[v] && (from < 0 || best[v] < best[from])) from = v;
        used[from] = true;
        if (result.parent[from] < 0) ++result.components;
        else result.weight += best[from];
        for (nidx_t to = 0; to < n; ++to)
            if (!used[to]) {
                W candidate = invoke(weight, from, to);
                if (candidate < best[to]) {
                    best[to] = move(candidate);
                    result.parent[to] = from;
                }
            }
    }
    return result;
}

template <class G, class K, class Cost, class W>
vector<W> nbellman_distances(G& graph, const K& source, Cost& cost, W infinity) {
    nidx_t n = graph.vertices.len();
    vector<W> distance(n, infinity);
    distance[graph.vertices.inverse(source)] = W{};
    vector<nidx_t> sources;
    for (nidx_t from = 0; from < n; ++from)
        if (begin(graph.edges(graph.vertices[from])) != end(graph.edges(graph.vertices[from])))
            sources.push_back(from);
    for (nidx_t pass = 1; pass < n; ++pass) {
        bool changed = false;
        for (nidx_t from : sources)
            if (distance[from] != infinity)
                for (auto&& edge : graph.edges(graph.vertices[from])) {
                    nidx_t to = graph.vertices.inverse(graph.target(edge));
                    W candidate = distance[from] + invoke(cost, edge);
                    changed |= nchmin(distance[to], candidate);
                }
        if (!changed) break;
    }
    return distance;
}

template <class G, class K, class Cost, class W>
optional<vector<W>> nbellman_ford(G&& graph, K source, Cost cost, W infinity) {
    auto distance = nbellman_distances(graph, source, cost, infinity);
    for (nidx_t from = 0; from < graph.vertices.len(); ++from)
        if (distance[from] != infinity)
            for (auto&& edge : graph.edges(graph.vertices[from])) {
                nidx_t to = graph.vertices.inverse(graph.target(edge));
                if (distance[from] + invoke(cost, edge) < distance[to]) return nullopt;
            }
    return distance;
}

template <class G, class K, class Cost, class W>
vector<W> nbellman_ford_closure(G&& graph, K source, Cost cost,
                                W infinity, W negative_infinity) {
    auto distance = nbellman_distances(graph, source, cost, infinity);
    nidx_t n = graph.vertices.len();
    vector<unsigned char> affected(n);
    vector<nidx_t> queue;
    auto mark = [&](nidx_t vertex) {
        if (!affected[vertex]) affected[vertex] = true, queue.push_back(vertex);
    };
    for (nidx_t from = 0; from < n; ++from)
        if (distance[from] != infinity)
            for (auto&& edge : graph.edges(graph.vertices[from])) {
                nidx_t to = graph.vertices.inverse(graph.target(edge));
                if (distance[from] + invoke(cost, edge) < distance[to]) mark(to);
            }
    for (nidx_t at = 0; at < nidx_t(queue.size()); ++at)
        for (auto&& edge : graph.edges(graph.vertices[queue[at]]))
            mark(graph.vertices.inverse(graph.target(edge)));
    for (nidx_t vertex : queue) distance[vertex] = negative_infinity;
    return distance;
}

template <class G, class K, class Cost, class W>
vector<W> ndijkstra(G&& graph, K source, Cost cost, W infinity) {
    vector<W> distance(graph.vertices.len(), infinity);
    priority_queue<pair<W, nidx_t>, vector<pair<W, nidx_t>>, greater<>> queue;
    nidx_t start = graph.vertices.inverse(source);
    distance[start] = W{};
    queue.emplace(W{}, start);
    while (!queue.empty()) {
        auto [current, from] = queue.top();
        queue.pop();
        if (distance[from] < current) continue;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            W candidate = current + invoke(cost, edge);
            if (candidate < distance[to]) {
                distance[to] = candidate;
                queue.emplace(candidate, to);
            }
        }
    }
    return distance;
}

template <class G, class K, class Cost>
vector<nidx_t> n01bfs(G&& graph, K source, Cost cost) {
    nidx_t n = graph.vertices.len(), start = graph.vertices.inverse(source);
    vector<nidx_t> distance(n, numeric_limits<nidx_t>::max());
    deque<pair<nidx_t, nidx_t>> queue;
    distance[start] = 0;
    queue.emplace_back(0, start);
    while (!queue.empty()) {
        auto [current, from] = queue.front();
        queue.pop_front();
        if (current != distance[from]) continue;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            nidx_t weight = invoke(cost, edge), candidate = current + weight;
            if (candidate >= distance[to]) continue;
            distance[to] = candidate;
            if (weight) queue.emplace_back(candidate, to);
            else queue.emplace_front(candidate, to);
        }
    }
    for (nidx_t& value : distance)
        if (value == numeric_limits<nidx_t>::max()) value = -1;
    return distance;
}

template <class G>
vector<nidx_t> ntoposort(G&& graph) {
    nidx_t n = graph.vertices.len();
    vector<nidx_t> indegree(n), queue, order;
    for (nidx_t from = 0; from < n; ++from)
        for (auto&& edge : graph.edges(graph.vertices[from]))
            ++indegree[graph.vertices.inverse(graph.target(edge))];
    for (nidx_t vertex = 0; vertex < n; ++vertex)
        if (!indegree[vertex]) queue.push_back(vertex);
    for (nidx_t at = 0; at < nidx_t(queue.size()); ++at) {
        nidx_t from = queue[at];
        order.push_back(from);
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            if (!--indegree[to]) queue.push_back(to);
        }
    }
    return order;
}

enum class neuler_kind { undirected, directed };

struct neuler_result {
    vector<nidx_t> vertices, edges;
    bool complete = false;
};

template <class G>
neuler_result neuler_run(G& graph, nidx_t m, nidx_t start, neuler_kind kind) {
    nidx_t n = graph.vertices.len();
    neuler_result result;
    if (m < 0) return result;
    vector<vector<pair<nidx_t, nidx_t>>> adjacency(n);
    vector<nidx_t> balance(n), cursor(n), ids;
    unordered_map<nidx_t, nidx_t> slots;
    slots.reserve(size_t(m));
    for (nidx_t from = 0; from < n; ++from)
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            nidx_t id = graph.edge_id(edge);
            auto [it, inserted] = slots.emplace(id, nidx_t(ids.size()));
            nidx_t slot = it->second;
            if (inserted) ids.push_back(id);
            adjacency[from].emplace_back(to, slot);
            if (kind == neuler_kind::directed) ++balance[from], --balance[to];
            else ++balance[from];
        }
    if (start < 0)
        for (nidx_t v = 0; v < n; ++v)
            if ((kind == neuler_kind::directed && balance[v] == 1) ||
                (kind == neuler_kind::undirected && (balance[v] & 1))) {
                start = v;
                break;
            }
    if (start < 0)
        for (nidx_t v = 0; v < n; ++v)
            if (!adjacency[v].empty()) { start = v; break; }
    if (start < 0 && n) start = 0;
    if (start < 0) {
        result.complete = nidx_t(ids.size()) == m;
        return result;
    }
    vector<unsigned char> used(ids.size());
    auto dfs = [&](auto&& self, nidx_t from) -> void {
        auto& bucket = adjacency[from];
        nidx_t& at = cursor[from];
        while (at < nidx_t(bucket.size())) {
            auto [to, slot] = bucket[at++];
            if (exchange(used[slot], true)) continue;
            self(self, to);
            result.edges.push_back(ids[slot]);
        }
        result.vertices.push_back(from);
    };
    dfs(dfs, start);
    ranges::reverse(result.vertices);
    ranges::reverse(result.edges);
    result.complete = nidx_t(ids.size()) == m && nidx_t(result.edges.size()) == m;
    return result;
}

template <class G>
neuler_result neuler(G&& graph, nidx_t edges,
                     neuler_kind kind = neuler_kind::undirected) {
    return neuler_run(graph, edges, -1, kind);
}

template <class G, class K>
neuler_result neuler(G&& graph, K&& source, nidx_t edges,
                     neuler_kind kind = neuler_kind::undirected) {
    return neuler_run(graph, edges, graph.vertices.inverse(forward<K>(source)), kind);
}

struct nlowlink_result {
    vector<unsigned char> articulation;
    vector<nidx_t> bridges;
};

struct nblockcut_result {
    nidx_t original_size = 0;
    vector<vector<nidx_t>> adjacency;
    vector<vector<nidx_t>> edges;
};

template <bool Blocks, class G>
auto nlowlink_run(G& graph) {
    nidx_t n = graph.vertices.len(), timer = 0;
    conditional_t<Blocks, nblockcut_result, nlowlink_result> result;
    if constexpr (Blocks) {
        result.original_size = n;
        result.adjacency.resize(n);
    } else {
        result.articulation.resize(n);
    }
    vector<nidx_t> dfn(n, -1), low(n), vertex_stack, edge_stack;
    auto dfs = [&](auto&& self, nidx_t from, nidx_t parent_edge) -> void {
        dfn[from] = low[from] = timer++;
        if constexpr (Blocks) vertex_stack.push_back(from);
        nidx_t children = 0;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            nidx_t id = graph.edge_id(edge);
            if (id == parent_edge || to == from) continue;
            if constexpr (Blocks)
                if (dfn[to] < 0 || dfn[to] < dfn[from]) edge_stack.push_back(id);
            if (dfn[to] < 0) {
                ++children;
                self(self, to, id);
                nchmin(low[from], low[to]);
                if constexpr (Blocks) {
                    if (low[to] >= dfn[from]) {
                        nidx_t block = nidx_t(result.adjacency.size());
                        result.adjacency.push_back({from});
                        result.adjacency[from].push_back(block);
                        result.edges.emplace_back();
                        nidx_t edge_id;
                        do {
                            edge_id = edge_stack.back();
                            edge_stack.pop_back();
                            result.edges.back().push_back(edge_id);
                        } while (edge_id != id);
                        nidx_t member;
                        do {
                            member = vertex_stack.back();
                            vertex_stack.pop_back();
                            result.adjacency[block].push_back(member);
                            result.adjacency[member].push_back(block);
                        } while (member != to);
                    }
                } else {
                    if (parent_edge >= 0 && low[to] >= dfn[from])
                        result.articulation[from] = true;
                    if (low[to] > dfn[from]) result.bridges.push_back(id);
                }
            } else {
                nchmin(low[from], dfn[to]);
            }
        }
        if (parent_edge < 0) {
            if constexpr (Blocks) {
                if (!children) {
                    nidx_t block = nidx_t(result.adjacency.size());
                    result.adjacency.push_back({from});
                    result.adjacency[from].push_back(block);
                    result.edges.emplace_back();
                }
                vertex_stack.pop_back();
            } else {
                result.articulation[from] = children > 1;
            }
        }
    };
    for (nidx_t from = 0; from < n; ++from)
        if (dfn[from] < 0) dfs(dfs, from, -1);
    return result;
}

template <class G>
nlowlink_result nlowlink(G&& graph) {
    return nlowlink_run<false>(graph);
}

template <class G>
nblockcut_result nblockcut(G&& graph) {
    return nlowlink_run<true>(graph);
}

struct nscc_result {
    vector<nidx_t> component;
    nidx_t count;
};

template <class G, class R>
nscc_result nscc(G&& graph, R&& reverse_graph) {
    nidx_t n = graph.vertices.len();
    vector<unsigned char> seen(n);
    vector<nidx_t> order;
    order.reserve(n);
    auto finish = [&](auto&& self, nidx_t from) -> void {
        seen[from] = true;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            if (!seen[to]) self(self, to);
        }
        order.push_back(from);
    };
    for (nidx_t source = 0; source < n; ++source)
        if (!seen[source]) finish(finish, source);

    vector<nidx_t> component(n, -1);
    nidx_t count = 0;
    auto assign = [&](auto&& self, nidx_t from) -> void {
        component[from] = count;
        for (auto&& edge : reverse_graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(reverse_graph.target(edge));
            if (component[to] < 0) self(self, to);
        }
    };
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        if (component[*it] >= 0) continue;
        assign(assign, *it);
        ++count;
    }
    return {move(component), count};
}
