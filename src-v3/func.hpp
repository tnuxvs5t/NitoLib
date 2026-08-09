#pragma once
#include "view.hpp"

/*
An nfunc separates enumeration from evaluation.  domain[i] is the semantic key at
position i; eval(key) is its value.  operator[] is positional, operator() is keyed.
The domain and evaluator are public movable descriptors so composition needs no holder,
friend protocol, locator trait or result-stabilization machinery.
*/
template <class D, class F>
struct nfunc {
    D domain;
    mutable F eval;

    constexpr int len() const { return domain.len(); }
    constexpr decltype(auto) key(int i) const { return domain[i]; }
    constexpr decltype(auto) operator[](int i) const { return invoke(eval, domain[i]); }

    template <class K>
    constexpr decltype(auto) operator()(K&& key) const {
        return invoke(eval, forward<K>(key));
    }
};

template <class D, class F>
nfunc(D, F) -> nfunc<D, F>;

/* Lvalue projections borrow the nfunc; rvalue projections consume and own it. */
template <class D, class F>
constexpr D& nkeys(nfunc<D, F>& function) { return function.domain; }

template <class D, class F>
constexpr const D& nkeys(const nfunc<D, F>& function) { return function.domain; }

template <class D, class F>
constexpr D nkeys(nfunc<D, F>&& function) { return move(function.domain); }

template <class D, class F>
constexpr D nkeys(const nfunc<D, F>&& function) { return function.domain; }

template <class G>
constexpr auto nvalues(G& function) {
    return nall(function);
}

template <class G>
constexpr auto nvalues(G&& function) {
    int n = function.len();
    return nview{n, [function = forward<G>(function)](int i) mutable -> decltype(auto) {
                     return function[i];
                 }};
}

template <class G>
constexpr auto nentries(G& function) {
    int n = function.len();
    return nview{n, [p = addressof(function)](int i) {
                     using K = decltype(p->key(i));
                     using V = decltype((*p)[i]);
                     return pair<K, V>(p->key(i), (*p)[i]);
                 }};
}

template <class G>
constexpr auto nentries(G&& function) {
    int n = function.len();
    return nview{n, [function = forward<G>(function)](int i) mutable {
                     using K = decltype(function.key(i));
                     using V = decltype(function[i]);
                     return pair<K, V>(function.key(i), function[i]);
                 }};
}

/* Re-domain changes only enumeration.  It performs no membership check. */
template <class G, class D>
constexpr auto nredomain(G function, D domain) {
    return nfunc{move(domain), move(function.eval)};
}

template <class G, class D>
constexpr auto nrestrict(G function, D domain) {
    return nredomain(move(function), move(domain));
}

/* nmap_values keeps keys and transforms values, preserving the transform's result. */
template <class G, class F>
constexpr auto nmap_values(G function, F transform) {
    auto domain = move(function.domain);
    auto eval = move(function.eval);
    return nfunc{move(domain),
                 [eval = move(eval), transform = move(transform)](auto&& key) mutable
                         -> decltype(auto) {
                     return invoke(transform, invoke(eval, forward<decltype(key)>(key)));
                 }};
}

/* outer(inner(key)); the resulting function enumerates inner's domain. */
template <class F, class G>
constexpr auto ncompose(F outer, G inner) {
    return nmap_values(move(inner), move(outer));
}

template <class G, class I>
constexpr auto nselect_positions(G function, I positions) {
    auto domain = ngather(move(function.domain), move(positions));
    return nfunc{move(domain), move(function.eval)};
}

/* locate(key) returns the position used to index values. */
template <class K, class V, class L>
constexpr auto nfunc_bind(K keys, V values, L locate) {
    return nfunc{move(keys),
                 [values = move(values), locate = move(locate)](auto&& key) mutable
                         -> decltype(auto) {
                     return values[invoke(locate, forward<decltype(key)>(key))];
                 }};
}

/* Dense ordinal binding is the common zero-locator case. */
template <class V>
constexpr auto nfunc_bind(V values) {
    int n = values.len();
    return nfunc{nrange(n), [values = move(values)](int i) mutable -> decltype(auto) {
                     return values[i];
                 }};
}

/*
nanchors materializes key -> position once, then owns that index beside the value
descriptor.  keys[i] corresponds to values[i].  Keys are unique, hash/equality agree,
both sequences have equal length, every queried key exists, and borrowed key/value
owners outlive the result.  Key length, order and values stay stable; payloads may mutate.
Construction is expected O(n), lookup expected O(1), and the index occupies O(n).
*/
template <class K, class V, class H, class E>
constexpr auto nanchors(K keys, V values, H hash, E equal) {
    using key_type = remove_cvref_t<decltype(keys[0])>;
    unordered_map<key_type, int, H, E> position(0, move(hash), move(equal));
    position.reserve(keys.len());
    for (int i = 0; i < keys.len(); ++i) position.emplace(keys[i], i);
    return nfunc{move(keys),
                 [values = move(values), position = move(position)](const auto& key) mutable
                         -> decltype(auto) {
                     return values[position.find(key)->second];
                 }};
}

template <class K, class V>
constexpr auto nanchors(K keys, V values) {
    using key_type = remove_cvref_t<decltype(keys[0])>;
    return nanchors(move(keys), move(values), hash<key_type>{}, equal_to<key_type>{});
}
