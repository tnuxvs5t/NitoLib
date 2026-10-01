#pragma once
#include "core.hpp"

struct nto_self {
    template <class E>
    constexpr decltype(auto) operator()(E&& edge) const {
        return forward<E>(edge);
    }
};

/* Dense integer domain; ngraph{n,next,to} needs no view or hash module. */
struct nvertices {
    nidx_t count;
    constexpr nvertices(nidx_t n) : count(n) {}
    constexpr nidx_t len() const { return count; }
    constexpr nidx_t operator[](nidx_t i) const { return i; }
    constexpr nidx_t inverse(nidx_t i) const { return i; }
};

/*
Minimal graph descriptor.  vertices enumerates semantic vertex keys, next(vertex)
returns any range-for compatible adjacency object, and to(edge) returns its target key.
vertices.inverse(key) returns its dense position.  No graph type, iterator category,
edge record or ownership model is imposed. Optional id(edge) gives a stable nonnegative
nidx_t logical edge ID, not an adjacency slot: opposite incidences of an undirected
edge share it, distinct edges never do. IDs may be sparse; -1 is reserved for no edge.
Algorithms requiring identity call edge_id; topology-only graphs have no such port.
*/
template <class V, class N, class To = nto_self, class Id = nullptr_t>
struct ngraph {
    V vertices;
    mutable N next;
    mutable To to{};
    [[no_unique_address]] mutable Id id{};

    template <class K>
    constexpr decltype(auto) edges(K&& vertex) const {
        return invoke(next, forward<K>(vertex));
    }

    template <class E>
    constexpr decltype(auto) target(E&& edge) const {
        return invoke(to, forward<E>(edge));
    }

    template <class E> requires invocable<Id&, E>
    constexpr nidx_t edge_id(E&& edge) const {
        return invoke(id, forward<E>(edge));
    }
};

template <class V, class N>
ngraph(V, N) -> ngraph<V, N>;

template <class V, class N, class To>
ngraph(V, N, To) -> ngraph<V, N, To>;

template <class V, class N, class To, class Id>
ngraph(V, N, To, Id) -> ngraph<V, N, To, Id>;

template <integral I, class N>
ngraph(I, N) -> ngraph<nvertices, N>;

template <integral I, class N, class To>
ngraph(I, N, To) -> ngraph<nvertices, N, To>;

template <integral I, class N, class To, class Id>
ngraph(I, N, To, Id) -> ngraph<nvertices, N, To, Id>;

/* Distances are stored by dense position.  Duplicate sources are harmless. */
template <class G, class R>
vector<nidx_t> nbfs_many(G&& graph, R&& sources) {
    vector<nidx_t> distance(graph.vertices.len(), -1), queue;
    queue.reserve(graph.vertices.len());
    for (nidx_t i = 0; i < nlen(sources); ++i) {
        nidx_t source = graph.vertices.inverse(sources[i]);
        if (distance[source] < 0) distance[source] = 0, queue.push_back(source);
    }
    for (nidx_t at = 0; at < nidx_t(queue.size()); ++at) {
        nidx_t from = queue[at];
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            if (distance[to] < 0) distance[to] = distance[from] + 1, queue.push_back(to);
        }
    }
    return distance;
}

template <class G, class K>
vector<nidx_t> nbfs(G&& graph, K source) {
    array<K, 1> sources{move(source)};
    return nbfs_many(forward<G>(graph), move(sources));
}
