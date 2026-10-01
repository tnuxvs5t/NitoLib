#pragma once
#include "graph.hpp"
#include "view.hpp"

/* Immutable CSR owner: source buckets preserve input order; ID is logical edge identity. */
template <class E, class To, class Id = nullptr_t>
struct ncsr {
    decltype(nrange(0)) vertices;
    vector<nidx_t> offset;
    vector<E> storage;
    [[no_unique_address]] mutable To to;
    [[no_unique_address]] mutable Id id;

    template <class V, class From>
    ncsr(nidx_t n, V&& edges, From from, To target, Id identity = {})
        : vertices(nrange(n)), offset(n + 1), to(move(target)), id(move(identity)) {
        for (nidx_t i = 0; i < nlen(edges); ++i)
            ++offset[invoke(from, edges[i]) + 1];
        partial_sum(offset.begin(), offset.end(), offset.begin());
        vector<nidx_t> cursor = offset;
        vector<optional<E>> slots(nlen(edges));
        for (nidx_t i = 0; i < nlen(edges); ++i) {
            nidx_t source = invoke(from, edges[i]);
            slots[cursor[source]++].emplace(edges[i]);
        }
        storage.reserve(nlen(edges));
        for (auto& edge : slots) storage.push_back(move(*edge));
    }

    auto edges(nidx_t vertex) {
        return nsub(nall(storage), offset[vertex], offset[vertex + 1]);
    }
    auto edges(nidx_t vertex) const {
        return nsub(nall(storage), offset[vertex], offset[vertex + 1]);
    }

    template <class X>
    decltype(auto) target(X&& edge) const {
        return invoke(to, forward<X>(edge));
    }

    template <class X>
    requires invocable<Id&, X>
    nidx_t edge_id(X&& edge) const {
        return invoke(id, forward<X>(edge));
    }

    auto view() {
        return ngraph{vertices, [this](nidx_t vertex) { return edges(vertex); },
                      ref(to), ref(id)};
    }
    auto view() const {
        return ngraph{vertices, [this](nidx_t vertex) { return edges(vertex); },
                      ref(to), ref(id)};
    }
};

template <class V, class From, class To, class Id = nullptr_t>
auto nmake_csr(nidx_t vertices, V&& edges, From from, To to, Id id = {}) {
    using E = remove_cvref_t<decltype(edges[0])>;
    return ncsr<E, To, Id>(vertices, forward<V>(edges), move(from), move(to), move(id));
}

struct ncsr_edge { nidx_t from, to, id; };

template <class V, class From, class To>
auto nmake_undirected_csr(nidx_t n, V&& edges, From from, To to) {
    vector<ncsr_edge> arcs;
    arcs.reserve(size_t(2) * nlen(edges));
    for (nidx_t i = 0; i < nlen(edges); ++i) {
        nidx_t u = invoke(from, edges[i]), v = invoke(to, edges[i]);
        arcs.push_back({u, v, i});
        arcs.push_back({v, u, i});
    }
    return nmake_csr(n, arcs, [](const auto& edge) { return edge.from; },
                     [](const auto& edge) { return edge.to; },
                     [](const auto& edge) { return edge.id; });
}
