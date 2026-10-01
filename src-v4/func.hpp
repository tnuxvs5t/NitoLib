#pragma once
#include "view.hpp"

template <class D, class F>
struct nfunc {
    D domain;
    mutable F eval;

    nidx_t len() const { return domain.len(); }
    decltype(auto) key(nidx_t i) const { return domain[i]; }
    decltype(auto) operator[](nidx_t i) const { return invoke(eval, domain[i]); }

    template <class K>
    decltype(auto) operator()(K&& key) const {
        return invoke(eval, forward<K>(key));
    }
};

template <class D, class F>
nfunc(D, F) -> nfunc<D, F>;

template <class D, class F>
auto nkeys(nfunc<D, F>& f) -> D& { return f.domain; }

template <class D, class F>
auto nkeys(const nfunc<D, F>& f) -> const D& { return f.domain; }

template <class G>
auto nvalues(G& f) { return nall(f); }

template <class G>
auto nvalues(G&& f) {
    return nview{f.len(), [f = forward<G>(f)](nidx_t i) mutable -> decltype(auto) {
        return f[i];
    }};
}

template <class G, class F>
auto nmap_values(G f, F transform) {
    return nfunc{move(f.domain), [eval = move(f.eval), transform = move(transform)](auto&& key)
        mutable -> decltype(auto) {
            return invoke(transform, invoke(eval, forward<decltype(key)>(key)));
        }};
}

template <class K, class V, class L>
auto nanchors(K keys, V values, L locate) {
    return nfunc{move(keys), [values = move(values), locate = move(locate)](auto&& key)
        mutable -> decltype(auto) {
            return values[invoke(locate, forward<decltype(key)>(key))];
        }};
}
