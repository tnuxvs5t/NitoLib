#pragma once
#include "graph.hpp"

/* Edge costs are nonnegative and every finite path sum must stay below infinity. */
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

/* Edge costs are exactly 0 or 1; unreachable positions are -1. */
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

/* Returns dense vertex positions.  A result shorter than vertices.len() exposes a cycle. */
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
