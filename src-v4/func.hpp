#pragma once
#include "hash.hpp"

/* nfunc keeps the three coordinates separate: position, semantic key, value. */
template <class D, class F>
struct nfunc {
    D domain;
    mutable F eval;

    constexpr nidx_t len() const { return domain.len(); }
    constexpr decltype(auto) key(nidx_t i) const { return domain[i]; }
    constexpr decltype(auto) operator[](nidx_t i) const {
        return invoke(eval, domain[i]);
    }

    template <class K>
    constexpr decltype(auto) operator()(K&& key) const {
        return invoke(eval, forward<K>(key));
    }

    template <class A, class B, class... C>
    constexpr decltype(auto) operator()(A&& first, B&& second, C&&... rest) const {
        if constexpr (sizeof...(C) == 0) {
            return invoke(eval, pair{forward<A>(first), forward<B>(second)});
        } else {
            return invoke(eval, tuple{forward<A>(first), forward<B>(second),
                                      forward<C>(rest)...});
        }
    }
};

template <class D, class F>
nfunc(D, F) -> nfunc<D, F>;

/* lvalues borrow their domain; rvalues transfer it, matching nvalues/nentries. */
template <class D, class F>
constexpr D& nkeys(nfunc<D, F>& f) { return f.domain; }

template <class D, class F>
constexpr const D& nkeys(const nfunc<D, F>& f) { return f.domain; }

template <class D, class F>
constexpr D nkeys(nfunc<D, F>&& f) { return move(f.domain); }

template <class D, class F>
constexpr D nkeys(const nfunc<D, F>&& f) { return f.domain; }

template <class G>
constexpr auto nvalues(G& f) {
    return nall(f);
}

template <class G>
constexpr auto nvalues(G&& f) {
    return nview{f.len(), [f = forward<G>(f)](nidx_t i) mutable -> decltype(auto) {
                     return f[i];
                 }};
}

template <class G>
constexpr auto nentries(G& f) {
    return nview{f.len(), [p = addressof(f)](nidx_t i) {
                     using K = decltype(p->key(i));
                     using V = decltype((*p)[i]);
                     return pair<K, V>(p->key(i), (*p)[i]);
                 }};
}

template <class G>
constexpr auto nentries(G&& f) {
    return nview{f.len(), [f = forward<G>(f)](nidx_t i) mutable {
                     using K = decltype(f.key(i));
                     using V = decltype(f[i]);
                     return pair<K, V>(f.key(i), f[i]);
                 }};
}

/* Re-domain changes enumeration only; membership remains the caller's contract. */
template <class G, class D>
constexpr auto nredomain(G f, D domain) {
    return nfunc{move(domain), move(f.eval)};
}

template <class G, class F>
constexpr auto nmap_values(G f, F transform) {
    auto domain = move(f.domain);
    auto eval = move(f.eval);
    return nfunc{
        move(domain),
        [eval = move(eval), transform = move(transform)](auto&& key) mutable
                -> decltype(auto) {
            return invoke(transform, invoke(eval, forward<decltype(key)>(key)));
        }
    };
}

template <class F, class G>
constexpr auto ncompose(F outer, G inner) {
    return nmap_values(move(inner), move(outer));
}

template <class K, class V, class L>
constexpr auto nanchors(K keys, V values, L locate) {
    return nfunc{
        move(keys),
        [values = move(values), locate = move(locate)](auto&& key) mutable
                -> decltype(auto) {
            return values[invoke(locate, forward<decltype(key)>(key))];
        }
    };
}

/* A structural inverse is cheaper and preserves key type; otherwise build once. */
template <class K, class V>
auto nanchors(K keys, V values) {
    if constexpr (copy_constructible<K> && requires(K& domain) {
                      domain.inverse(domain[0]);
                  }) {
        K locate = keys;
        return nfunc{
            move(keys),
            [values = move(values), locate = move(locate)](auto&& key) mutable
                    -> decltype(auto) {
                return values[locate.inverse(forward<decltype(key)>(key))];
            }
        };
    } else {
        auto locate = nmake_hash_inverse(keys);
        return nfunc{
            move(keys),
            [values = move(values), locate = move(locate)](const auto& key) mutable
                    -> decltype(auto) {
                return values[locate.find(key)];
            }
        };
    }
}

template <class K, class V, class H, class E>
auto nanchors(K keys, V values, H hash, E equal) {
    auto locate = nmake_hash_inverse(keys, move(hash), move(equal));
    return nfunc{
        move(keys),
        [values = move(values), locate = move(locate)](const auto& key) mutable
                -> decltype(auto) {
            return values[locate.find(key)];
        }
    };
}

template <class V>
constexpr auto nanchors(V values) {
    nidx_t n = values.len();
    return nfunc{
        nrange(n),
        [values = move(values)](nidx_t i) mutable -> decltype(auto) {
            return values[i];
        }
    };
}
