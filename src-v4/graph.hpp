#pragma once
#include "core.hpp"

struct nto_self {
    template <class E>
    constexpr decltype(auto) operator()(E&& edge) const {
        return forward<E>(edge);
    }
};

template <class V, class N, class To = nto_self>
struct ngraph {
    V vertices;
    mutable N next;
    mutable To to{};

    nidx_t len() const { return vertices.len(); }

    template <class X>
    decltype(auto) edges(X&& vertex) const {
        return invoke(next, vertices[forward<X>(vertex)]);
    }

    template <class E>
    decltype(auto) target(E&& edge) const {
        return invoke(to, forward<E>(edge));
    }
};

template <class V, class N>
ngraph(V, N) -> ngraph<V, N>;

template <class V, class N, class To>
ngraph(V, N, To) -> ngraph<V, N, To>;

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
