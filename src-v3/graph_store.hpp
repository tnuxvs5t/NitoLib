#pragma once
#include "graph.hpp"

/*
Immutable CSR owner for dense integer vertices.  From and To are projections on the
stored edge record; construction is O(V+E) and preserves input order inside each source
bucket.  Returned adjacency nviews borrow this owner and expire when it moves or dies.
*/
template <class E, class To>
struct ncsr {
    decltype(nrange(0)) vertices;
    vector<nidx_t> offset;
    vector<E> storage;
    [[no_unique_address]] mutable To to;

    template <class V, class From>
    ncsr(nidx_t n, V edges, From from, To target)
        : vertices(nrange(n)), offset(n + 1), to(move(target)) {
        for (nidx_t i = 0; i < edges.len(); ++i) ++offset[invoke(from, edges[i]) + 1];
        partial_sum(offset.begin(), offset.end(), offset.begin());
        vector<nidx_t> cursor = offset;
        vector<optional<E>> slots(edges.len());
        for (nidx_t i = 0; i < edges.len(); ++i) {
            nidx_t source = invoke(from, edges[i]);
            slots[cursor[source]++].emplace(edges[i]);
        }
        storage.reserve(edges.len());
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

    auto view() {
        return ngraph{vertices, [this](nidx_t vertex) { return edges(vertex); },
                      [this](auto&& edge) -> decltype(auto) {
                          return target(forward<decltype(edge)>(edge));
                      }};
    }
    auto view() const {
        return ngraph{vertices, [this](nidx_t vertex) { return edges(vertex); },
                      [this](auto&& edge) -> decltype(auto) {
                          return target(forward<decltype(edge)>(edge));
                      }};
    }
};

template <class V, class From, class To>
auto nmake_csr(nidx_t vertices, V edges, From from, To to) {
    using E = remove_cvref_t<decltype(edges[0])>;
    return ncsr<E, To>(vertices, move(edges), move(from), move(to));
}
