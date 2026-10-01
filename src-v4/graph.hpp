#pragma once
#include "core.hpp"

struct nto_self {
    template <class E>
    constexpr decltype(auto) operator()(E&& edge) const {
        return forward<E>(edge);
    }
};

template <class V, class N, class To = nto_self, class Id = nullptr_t>
struct ngraph {
    V vertices;
    mutable N next;
    mutable To to{};
    mutable Id id{};

    nidx_t len() const { return vertices.len(); }

    template <class X>
    decltype(auto) edges(X&& vertex) const {
        return invoke(next, vertices[forward<X>(vertex)]);
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
