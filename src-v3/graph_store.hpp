#pragma once
#include "graph.hpp"
#include "view.hpp"

/*
Immutable CSR owner for dense integer vertices.  From and To are projections on the
stored edge record; construction is O(V+E) and preserves input order inside each source
bucket.  Returned adjacency nviews borrow this owner and expire when it moves or dies.
Optional Id projects the logical edge identity from the record, never the CSR slot.
*/
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
        for (nidx_t i = 0; i < nlen(edges); ++i) ++offset[invoke(from, edges[i]) + 1];
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
    template <class X> requires invocable<Id&, X>
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

/*
Each input position is ONE undirected edge on [0,n). Expand it to two incidences
with shared id == input position (including loops). O(V+E) time/space; 2E fits nidx_t.
Only endpoints and IDs are owned; payloads remain external, accessible by input ID.
No input record is copied or borrowed. Positions refer to this input enumeration.
*/
template <class V, class From, class To>
auto nmake_undirected_csr(nidx_t n, V&& edges, From from, To to) {
    vector<ncsr_edge> arcs;
    arcs.reserve(2 * nlen(edges));
    for (nidx_t i = 0; i < nlen(edges); ++i) {
        nidx_t u = invoke(from, edges[i]), v = invoke(to, edges[i]);
        arcs.push_back({u, v, i});
        arcs.push_back({v, u, i});
    }
    return nmake_csr(n, arcs, [](const auto& e) { return e.from; },
                     [](const auto& e) { return e.to; }, [](const auto& e) { return e.id; });
}
