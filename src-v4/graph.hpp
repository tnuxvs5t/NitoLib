#pragma once
#include "core.hpp"

struct nto_self {
    template <class E>
    constexpr decltype(auto) operator()(E&& edge) const {
        return forward<E>(edge);
    }
};

// vertices[position] is a key; edges(key) and target(edge) use that key domain.
// Opposite incidences of an undirected edge share one nonnegative edge_id;
// distinct logical edges have distinct IDs. -1 denotes no edge.
template <class V, class N, class To = nto_self, class Id = nullptr_t>
struct ngraph {
    V vertices;
    mutable N next;
    mutable To to{};
    mutable Id id{};

    nidx_t len() const { return vertices.len(); }

    template <class X>
    decltype(auto) edges(X&& vertex) const {
        return invoke(next, forward<X>(vertex));
    }

    template <class E>
    decltype(auto) target(E&& edge) const {
        return invoke(to, forward<E>(edge));
    }

    template <class E>
    requires invocable<Id&, E>
    nidx_t edge_id(E&& edge) const {
        return invoke(id, forward<E>(edge));
    }
};

template <class V, class N>
ngraph(V, N) -> ngraph<V, N>;

template <class V, class N, class To>
ngraph(V, N, To) -> ngraph<V, N, To>;

template <class V, class N, class To, class Id>
ngraph(V, N, To, Id) -> ngraph<V, N, To, Id>;

struct nvertices {
    nidx_t n;
    nidx_t len() const { return n; }
    nidx_t operator[](nidx_t v) const { return v; }
    nidx_t inverse(nidx_t v) const { return v; }
};

template <integral I, class N>
ngraph(I, N) -> ngraph<nvertices, N>;

template <integral I, class N, class To>
ngraph(I, N, To) -> ngraph<nvertices, N, To>;

template <integral I, class N, class To, class Id>
ngraph(I, N, To, Id) -> ngraph<nvertices, N, To, Id>;

// Distances use dense positions; unreachable vertices are -1, duplicate sources
// are harmless. O(V+E+S) time for S sources, O(V) space, with unit-cost port calls.
template <class G, class R>
vector<nidx_t> nbfs_many(G&& graph, R&& sources) {
    vector<nidx_t> distance(graph.vertices.len(), -1), queue;
    queue.reserve(distance.size());
    for (nidx_t i = 0; i < nlen(sources); ++i) {
        nidx_t v = graph.vertices.inverse(sources[i]);
        if (distance[v] >= 0) continue;
        distance[v] = 0;
        queue.push_back(v);
    }
    for (size_t at = 0; at < queue.size(); ++at) {
        nidx_t v = queue[at];
        for (auto&& edge : graph.edges(graph.vertices[v])) {
            nidx_t u = graph.vertices.inverse(graph.target(edge));
            if (distance[u] >= 0) continue;
            distance[u] = distance[v] + 1;
            queue.push_back(u);
        }
    }
    return distance;
}

template <class G, class K>
vector<nidx_t> nbfs(G&& graph, K source) {
    return nbfs_many(forward<G>(graph), array<K, 1>{move(source)});
}
