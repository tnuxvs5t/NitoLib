#pragma once
#include "graph.hpp"

template <class W> struct ndenseprim_result {
    W weight;
    vector<nidx_t> parent;
    nidx_t components;
};

/*
Minimum spanning forest on n >= 0 dense positions, with symmetric, stable weight(u,v).
infinity denotes a missing edge; every finite edge weight is strictly below it.
Negative weights are allowed, self-loops are ignored. W{} is zero; comparison and
addition are exact and all accumulated sums fit W (sums need not be below infinity).
parent[v] == -1 marks each component root; other entries identify selected edges.
The empty graph has zero weight/components. O(n^2) time, O(n) extra space for
constant-time weight/arithmetic; each unordered distinct pair is queried once.
No adjacency storage, inverse map or materialized matrix is required.
*/
template <class Weight, class W> ndenseprim_result<W> ndenseprim(nidx_t n, Weight&& weight, W infinity) {
    ndenseprim_result<W> result{W{}, vector<nidx_t>(n, -1), 0};
    vector<W> best(n, infinity);
    vector<unsigned char> used(n);
    for (nidx_t step = 0; step < n; ++step) {
        nidx_t from = -1;
        for (nidx_t v = 0; v < n; ++v)
            if (!used[v] && (from < 0 || best[v] < best[from]))
                from = v;
        used[from] = true;
        if (result.parent[from] < 0)
            ++result.components;
        else
            result.weight += best[from];
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

namespace ngraph_algo_detail {
/* In-place full-edge passes; after pass k all paths of at most k edges are covered. */
template <class G, class K, class Cost, class W>
vector<W> bellman_ford_distances(G& graph, const K& source, Cost& cost, W infinity) {
    nidx_t n = graph.vertices.len();
    vector<W> distance(n, infinity);
    distance[graph.vertices.inverse(source)] = W{};
    vector<nidx_t> sources;
    for (nidx_t from = 0; from < n; ++from) {
        auto&& edges = graph.edges(graph.vertices[from]);
        if (begin(edges) != end(edges))
            sources.push_back(from);
    }
    for (nidx_t pass = 1; pass < n; ++pass) {
        bool changed = false;
        for (nidx_t from : sources)
            if (distance[from] != infinity)
                for (auto&& edge : graph.edges(graph.vertices[from])) {
                    nidx_t to = graph.vertices.inverse(graph.target(edge));
                    W candidate = distance[from] + invoke(cost, edge);
                    changed |= nchmin(distance[to], candidate);
                }
        if (!changed)
            break;
    }
    return distance;
}
} // namespace ngraph_algo_detail

/*
Signed edge costs; source is a vertex key in a nonempty, stable graph. Returns distances
by dense position, with infinity for unreachable vertices, or nullopt iff a negative
cycle is reachable from source. Uses V-1 full-edge passes (early exit if unchanged),
then a read-only detection pass: O(V+VE) time, O(V) space with constant-time ports.
Only nonempty adjacency sources are rescanned, so isolated vertices cost O(V) total.
W{} is zero; comparison/addition must be exact and every evaluated finite sum must be
representable and strictly below infinity, including walks during negative-cycle
detection. No arithmetic on infinity, clamping or implicit widening is performed.
*/
template <class G, class K, class Cost, class W>
optional<vector<W>> nbellman_ford(G&& graph, K source, Cost cost, W infinity) {
    auto distance = ngraph_algo_detail::bellman_ford_distances(graph, source, cost, infinity);
    for (nidx_t from = 0; from < graph.vertices.len(); ++from)
        if (distance[from] != infinity)
            for (auto&& edge : graph.edges(graph.vertices[from])) {
                nidx_t to = graph.vertices.inverse(graph.target(edge));
                W candidate = distance[from] + invoke(cost, edge);
                if (candidate < distance[to])
                    return nullopt;
            }
    return distance;
}

/*
Same graph/arithmetic contract as nbellman_ford. The shortest-walk closure distinguishes
unreachable (+infinity), finite distance, and unbounded below (negative_infinity).
The two sentinels are distinct and outside all evaluated finite sums. A read-only scan
seeds targets of still-relaxable edges, then a one-visit reachability traversal marks
their forward closure. No distance relaxation uses a queue or either sentinel.
O(V+VE) time, O(V) space; unaffected distances remain valid even with negative cycles.
*/
template <class G, class K, class Cost, class W>
vector<W> nbellman_ford_closure(G&& graph, K source, Cost cost, W infinity, W negative_infinity) {
    auto distance = ngraph_algo_detail::bellman_ford_distances(graph, source, cost, infinity);
    nidx_t n = graph.vertices.len();
    vector<unsigned char> affected(n);
    vector<nidx_t> queue;
    auto mark = [&](nidx_t vertex) {
        if (!affected[vertex])
            affected[vertex] = true, queue.push_back(vertex);
    };
    for (nidx_t from = 0; from < n; ++from)
        if (distance[from] != infinity)
            for (auto&& edge : graph.edges(graph.vertices[from])) {
                nidx_t to = graph.vertices.inverse(graph.target(edge));
                W candidate = distance[from] + invoke(cost, edge);
                if (candidate < distance[to])
                    mark(to);
            }
    for (nidx_t at = 0; at < nidx_t(queue.size()); ++at)
        for (auto&& edge : graph.edges(graph.vertices[queue[at]]))
            mark(graph.vertices.inverse(graph.target(edge)));
    for (nidx_t vertex : queue)
        distance[vertex] = negative_infinity;
    return distance;
}

/* Edge costs are nonnegative and every finite path sum must stay below infinity. */
template <class G, class K, class Cost, class W> vector<W> ndijkstra(G&& graph, K source, Cost cost, W infinity) {
    vector<W> distance(graph.vertices.len(), infinity);
    priority_queue<pair<W, nidx_t>, vector<pair<W, nidx_t>>, greater<>> queue;
    nidx_t start = graph.vertices.inverse(source);
    distance[start] = W{};
    queue.emplace(W{}, start);
    while (!queue.empty()) {
        auto [current, from] = queue.top();
        queue.pop();
        if (distance[from] < current)
            continue;
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

/* Edge costs are exactly 0 or 1; unreachable positions are -1. */
template <class G, class K, class Cost> vector<nidx_t> n01bfs(G&& graph, K source, Cost cost) {
    nidx_t n = graph.vertices.len(), start = graph.vertices.inverse(source);
    vector<nidx_t> distance(n, numeric_limits<nidx_t>::max());
    deque<pair<nidx_t, nidx_t>> queue;
    distance[start] = 0;
    queue.emplace_back(0, start);
    while (!queue.empty()) {
        auto [current, from] = queue.front();
        queue.pop_front();
        if (current != distance[from])
            continue;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            nidx_t weight = invoke(cost, edge), candidate = current + weight;
            if (candidate >= distance[to])
                continue;
            distance[to] = candidate;
            if (weight)
                queue.emplace_back(candidate, to);
            else
                queue.emplace_front(candidate, to);
        }
    }
    for (nidx_t& value : distance)
        if (value == numeric_limits<nidx_t>::max())
            value = -1;
    return distance;
}

/* Returns dense vertex positions.  A result shorter than vertices.len() exposes a cycle. */
template <class G> vector<nidx_t> ntoposort(G&& graph) {
    nidx_t n = graph.vertices.len();
    vector<nidx_t> indegree(n), queue, order;
    for (nidx_t from = 0; from < n; ++from)
        for (auto&& edge : graph.edges(graph.vertices[from]))
            ++indegree[graph.vertices.inverse(graph.target(edge))];
    for (nidx_t vertex = 0; vertex < n; ++vertex)
        if (!indegree[vertex])
            queue.push_back(vertex);
    for (nidx_t at = 0; at < nidx_t(queue.size()); ++at) {
        nidx_t from = queue[at];
        order.push_back(from);
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            if (!--indegree[to])
                queue.push_back(to);
        }
    }
    return order;
}

enum class neuler_kind { undirected, directed };

struct neuler_result {
    vector<nidx_t> vertices, edges;
    bool complete = false;
};

/* Unchecked recursive Hierholzer from the chosen start. The caller ensures each
   logical edge has one directed incidence or two opposite undirected incidences
   (including two incidences for a loop), and that the chosen start's edge component
   admits an Euler trail from it. Equal IDs denote the same logical edge; IDs may be
   sparse. Other components are not traversed. complete only compares the input and
   visited distinct-ID counts with m; it does not certify a valid walk. Without
   the Euler preconditions, adjacent entries in vertices/edges need not form a walk.
   Returns dense vertex positions and original IDs. Expected O(V+A) time, O(V+A)
   space with constant-time ports; DFS depth is at most distinct edge IDs + 1. */
namespace ngraph_algo_detail {
template <class G> neuler_result euler(G& graph, nidx_t m, nidx_t start, neuler_kind kind) {
    nidx_t n = graph.vertices.len();
    neuler_result result;
    if (m < 0)
        return result;
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
            if (inserted)
                ids.push_back(id);
            adjacency[from].emplace_back(to, slot);
            if (kind == neuler_kind::directed) {
                ++balance[from];
                --balance[to];
            } else {
                ++balance[from];
            }
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
            if (!adjacency[v].empty()) {
                start = v;
                break;
            }
    if (start < 0 && n)
        start = 0;
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
            if (exchange(used[slot], true))
                continue;
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
} // namespace ngraph_algo_detail

template <class G> neuler_result neuler(G&& graph, nidx_t edges, neuler_kind kind = neuler_kind::undirected) {
    return ngraph_algo_detail::euler(graph, edges, -1, kind);
}

template <class G, class K>
neuler_result neuler(G&& graph, K&& source, nidx_t edges, neuler_kind kind = neuler_kind::undirected) {
    return ngraph_algo_detail::euler(graph, edges, graph.vertices.inverse(forward<K>(source)), kind);
}

struct nlowlink_result {
    vector<unsigned char> articulation;
    vector<nidx_t> bridges;
};

struct nblockcut_result {
    nidx_t original_size;
    vector<vector<nidx_t>> adjacency;
    vector<vector<nidx_t>> edges;
};

namespace ngraph_algo_detail {
// One traversal, two outputs. Cuts-only never allocates either stack.
template <bool Blocks, class G> auto lowlink(G& graph) {
    nidx_t n = graph.vertices.len(), timer = 0;
    conditional_t<Blocks, nblockcut_result, nlowlink_result> result;
    if constexpr (Blocks) {
        result.original_size = n;
        result.adjacency.resize(n);
    } else
        result.articulation.resize(n);
    vector<nidx_t> dfn(n, -1), low(n), stack, edge_stack;
    auto dfs = [&](auto&& self, nidx_t from, nidx_t parent_edge) -> void {
        dfn[from] = low[from] = timer++;
        if constexpr (Blocks)
            stack.push_back(from);
        nidx_t children = 0;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            nidx_t id = graph.edge_id(edge);
            if (id == parent_edge || to == from)
                continue;
            // Each non-loop edge enters once: tree descent or descendant-to-ancestor.
            if constexpr (Blocks)
                if (dfn[to] < 0 || dfn[to] < dfn[from])
                    edge_stack.push_back(id);
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
                        nidx_t member_edge;
                        do {
                            member_edge = edge_stack.back();
                            edge_stack.pop_back();
                            result.edges.back().push_back(member_edge);
                        } while (member_edge != id);
                        // Keep from on the stack: it can belong to further blocks.
                        nidx_t member;
                        do {
                            member = stack.back();
                            stack.pop_back();
                            result.adjacency[block].push_back(member);
                            result.adjacency[member].push_back(block);
                        } while (member != to);
                    }
                } else {
                    if (parent_edge >= 0 && low[to] >= dfn[from])
                        result.articulation[from] = true;
                    if (low[to] > dfn[from])
                        result.bridges.push_back(id);
                }
            } else
                nchmin(low[from], dfn[to]);
        }
        if (parent_edge < 0) {
            if constexpr (Blocks) {
                if (!children) {
                    nidx_t block = nidx_t(result.adjacency.size());
                    result.adjacency.push_back({from});
                    result.adjacency[from].push_back(block);
                    result.edges.emplace_back();
                }
                stack.pop_back();
            } else
                result.articulation[from] = children > 1;
        }
    };
    for (nidx_t from = 0; from < n; ++from)
        if (dfn[from] < 0)
            dfs(dfs, from, -1);
    return result;
}
} // namespace ngraph_algo_detail

/*
Tarjan cut vertices and bridges of a stable undirected multigraph, including all
components. Requires edge_id(edge): distinct logical edges have distinct nonnegative
nidx_t IDs; the two incidences of each non-loop edge share an ID and opposite endpoints.
Loops may occur once or twice. IDs may be sparse; -1 means no parent edge. Skip that
edge, not the parent vertex. Directed/asymmetric input or reused IDs are invalid.
articulation[v] is 0/1 by dense position; bridges are original logical edge IDs in
DFS completion order, each once. Parallel edges and loops are never bridges.
With constant-time ports: O(V+E) time, O(V) extra space including output and recursive
call stack. Outstanding adjacency ranges/iterators must survive nested calls.
*/
template <class G> nlowlink_result nlowlink(G&& graph) {
    return ngraph_algo_detail::lowlink<false>(graph);
}

/*
Block-cut forest of the same stable undirected multigraph accepted by nlowlink.
All original dense positions [0,n) remain vertices; [n,adjacency.size()) are block
nodes, joined once to each member. edges[b-n] lists the logical edge IDs in block b,
each non-loop edge exactly once overall. Loops are deliberately excluded from both
the forest and edge lists; vertices isolated after ignoring
loops receive singleton blocks. Bridges form two-member blocks, but parallel edges
can also form two-member blocks. Each original component becomes one tree; an original
vertex is a cut vertex iff its forest degree exceeds one. Empty input gives n == 0
and empty adjacency/edge lists. Block/member order follows DFS and is not canonical.
The result owns vertex positions and edge IDs, not keys or records; it survives the input.
The total forest node count must fit nidx_t. With constant-time ports: O(V+E) time,
O(V+E) space including edge lists/stacks, result and recursive call stack, independent
of the maximum ID. Singleton blocks have empty edge lists. Nested adjacency ranges/
iterators must remain valid. No edge-record copying or cuts-only preliminary pass.
*/
template <class G> nblockcut_result nblockcut(G&& graph) {
    return ngraph_algo_detail::lowlink<true>(graph);
}

struct nscc_result {
    vector<nidx_t> component;
    nidx_t count;
};

/*
Kosaraju receives both forward and reverse descriptors over the same vertex keys;
their enumeration orders may differ.  Results use forward-graph positions.
This keeps the graph port minimal and lets CSR/forward-star callers choose whether and
how reverse edges are stored.  Component labels are dense in second-pass discovery order.
Recursive DFS uses O(V) call stack; outstanding adjacency ranges survive nested calls.
*/
template <class G, class R> nscc_result nscc(G&& graph, R&& reverse_graph) {
    nidx_t n = graph.vertices.len();
    vector<unsigned char> seen(n);
    vector<nidx_t> order;
    order.reserve(n);
    auto finish = [&](auto&& self, nidx_t from) -> void {
        seen[from] = true;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            if (!seen[to])
                self(self, to);
        }
        order.push_back(from);
    };
    for (nidx_t source = 0; source < n; ++source)
        if (!seen[source])
            finish(finish, source);

    vector<nidx_t> component(n, -1);
    nidx_t count = 0;
    auto assign = [&](auto&& self, nidx_t from) -> void {
        component[from] = count;
        for (auto&& edge : reverse_graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(reverse_graph.target(edge));
            if (component[to] < 0)
                self(self, to);
        }
    };
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        if (component[*it] >= 0)
            continue;
        assign(assign, *it);
        ++count;
    }
    return {move(component), count};
}
