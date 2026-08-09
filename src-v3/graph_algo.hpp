#pragma once
#include "graph.hpp"

/* Edge costs are nonnegative and every finite path sum must stay below infinity. */
template <class G, class K, class Cost, class W, class Id = nordinal>
vector<W> ndijkstra(G&& graph, K source, Cost cost, W infinity, Id index = {}) {
    vector<W> distance(graph.vertices.len(), infinity);
    priority_queue<pair<W, int>, vector<pair<W, int>>, greater<>> queue;
    int start = invoke(index, source);
    distance[start] = W{};
    queue.emplace(W{}, start);
    while (!queue.empty()) {
        auto [current, from] = queue.top();
        queue.pop();
        if (distance[from] < current) continue;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            int to = invoke(index, graph.target(edge));
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
template <class G, class K, class Cost, class Id = nordinal>
vector<int> n01bfs(G&& graph, K source, Cost cost, Id index = {}) {
    int n = graph.vertices.len(), start = invoke(index, source);
    vector<int> distance(n, numeric_limits<int>::max());
    deque<pair<int, int>> queue;
    distance[start] = 0;
    queue.emplace_back(0, start);
    while (!queue.empty()) {
        auto [current, from] = queue.front();
        queue.pop_front();
        if (current != distance[from]) continue;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            int to = invoke(index, graph.target(edge));
            int weight = invoke(cost, edge), candidate = current + weight;
            if (candidate >= distance[to]) continue;
            distance[to] = candidate;
            if (weight) queue.emplace_back(candidate, to);
            else queue.emplace_front(candidate, to);
        }
    }
    for (int& value : distance)
        if (value == numeric_limits<int>::max()) value = -1;
    return distance;
}

/* Returns dense vertex positions.  A result shorter than vertices.len() exposes a cycle. */
template <class G, class Id = nordinal>
vector<int> ntoposort(G&& graph, Id index = {}) {
    int n = graph.vertices.len();
    vector<int> indegree(n), queue, order;
    for (int from = 0; from < n; ++from)
        for (auto&& edge : graph.edges(graph.vertices[from]))
            ++indegree[invoke(index, graph.target(edge))];
    for (int vertex = 0; vertex < n; ++vertex)
        if (!indegree[vertex]) queue.push_back(vertex);
    for (int at = 0; at < int(queue.size()); ++at) {
        int from = queue[at];
        order.push_back(from);
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            int to = invoke(index, graph.target(edge));
            if (!--indegree[to]) queue.push_back(to);
        }
    }
    return order;
}

struct nscc_result {
    vector<int> component;
    int count;
};

/*
Kosaraju receives both forward and reverse descriptors over the same vertex keys.
This keeps the graph port minimal and lets CSR/forward-star callers choose whether and
how reverse edges are stored.  Component labels are dense in second-pass discovery order.
*/
template <class G, class R, class Id = nordinal>
nscc_result nscc(G&& graph, R&& reverse_graph, Id index = {}) {
    int n = graph.vertices.len();
    vector<unsigned char> seen(n);
    vector<int> order;
    order.reserve(n);
    for (int source = 0; source < n; ++source) {
        if (seen[source]) continue;
        vector<pair<int, bool>> stack{{source, false}};
        while (!stack.empty()) {
            auto [from, exit] = stack.back();
            stack.pop_back();
            if (exit) {
                order.push_back(from);
                continue;
            }
            if (seen[from]) continue;
            seen[from] = true;
            stack.emplace_back(from, true);
            for (auto&& edge : graph.edges(graph.vertices[from])) {
                int to = invoke(index, graph.target(edge));
                if (!seen[to]) stack.emplace_back(to, false);
            }
        }
    }

    vector<int> component(n, -1), stack;
    int count = 0;
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        if (component[*it] >= 0) continue;
        component[*it] = count;
        stack.push_back(*it);
        while (!stack.empty()) {
            int from = stack.back();
            stack.pop_back();
            for (auto&& edge : reverse_graph.edges(reverse_graph.vertices[from])) {
                int to = invoke(index, reverse_graph.target(edge));
                if (component[to] < 0) component[to] = count, stack.push_back(to);
            }
        }
        ++count;
    }
    return {move(component), count};
}
