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

    template <class A, class B, class... C>
    decltype(auto) operator()(A&& first, B&& second, C&&... rest) const {
        if constexpr (sizeof...(C) == 0)
            return invoke(eval, pair{forward<A>(first), forward<B>(second)});
        else
            return invoke(eval, tuple{forward<A>(first), forward<B>(second),
                                      forward<C>(rest)...});
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

template <class G>
auto nentries(G& f) {
    return nview{f.len(), [p = addressof(f)](nidx_t i) {
        return pair{p->key(i), (*p)[i]};
    }};
}

template <class G>
auto nentries(G&& f) {
    return nview{f.len(), [f = forward<G>(f)](nidx_t i) mutable {
        return pair{f.key(i), f[i]};
    }};
}

template <class G, class D>
auto nredomain(G f, D domain) {
    return nfunc{move(domain), move(f.eval)};
}

template <class F, class G>
auto ncompose(F outer, G inner) {
    return nmap_values(move(inner), move(outer));
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

template <class K, class V>
requires copy_constructible<K> && requires(K& keys, decltype(keys[0]) key) {
    keys.inverse(key);
}
auto nanchors(K keys, V values) {
    K locate = keys;
    return nanchors(move(keys), move(values),
                    [locate = move(locate)](auto&& key) mutable {
                        return locate.inverse(forward<decltype(key)>(key));
                    });
}
