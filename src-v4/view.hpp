#pragma once
#include "core.hpp"

/*
   nview is a finite positional projection.  It owns only its accessor; anything
   reached through that accessor remains borrowed unless the accessor owns it.
   Const is shallow on purpose: a const descriptor may still yield T&.
*/
template <class A>
struct nview {
    nidx_t length;
    mutable A access;

    constexpr nidx_t len() const { return length; }
    constexpr bool empty() const { return length == 0; }
    constexpr decltype(auto) operator[](nidx_t i) const { return invoke(access, i); }

    template <class K>
    requires requires(A& a, K&& key) { a.inverse(forward<K>(key)); }
    constexpr decltype(auto) inverse(K&& key) const {
        return access.inverse(forward<K>(key));
    }

    struct iterator {
        using difference_type = nidx_t;
        using reference = decltype(declval<const nview&>()[0]);
        using value_type = remove_cvref_t<reference>;
        using pointer = void;
        using iterator_category = random_access_iterator_tag;
        using iterator_concept = random_access_iterator_tag;

        const nview* view = nullptr;
        nidx_t position = 0;

        constexpr decltype(auto) operator*() const { return (*view)[position]; }
        constexpr decltype(auto) operator[](nidx_t d) const {
            return (*view)[position + d];
        }
        constexpr iterator& operator++() { ++position; return *this; }
        constexpr iterator operator++(int) { auto old = *this; ++*this; return old; }
        constexpr iterator& operator--() { --position; return *this; }
        constexpr iterator operator--(int) { auto old = *this; --*this; return old; }
        constexpr iterator& operator+=(nidx_t d) { position += d; return *this; }
        constexpr iterator& operator-=(nidx_t d) { position -= d; return *this; }
        friend constexpr iterator operator+(iterator it, nidx_t d) { return it += d; }
        friend constexpr iterator operator+(nidx_t d, iterator it) { return it += d; }
        friend constexpr iterator operator-(iterator it, nidx_t d) { return it -= d; }
        constexpr nidx_t operator-(iterator other) const { return position - other.position; }
        constexpr auto operator<=>(const iterator& other) const {
            return position <=> other.position;
        }
        constexpr bool operator==(const iterator& other) const {
            return view == other.view && position == other.position;
        }
    };

    constexpr iterator begin() const { return {this, 0}; }
    constexpr iterator end() const { return {this, length}; }
};

template <class A>
nview(nidx_t, A) -> nview<A>;

/* Hash fallback keys own the value category recursively, including pair/tuple refs. */
namespace ndetail {

template <class T>
struct nowned {
    using type = remove_cvref_t<T>;
};

template <class A, class B>
struct nowned<pair<A, B>> {
    using type = pair<typename nowned<remove_cvref_t<A>>::type,
                      typename nowned<remove_cvref_t<B>>::type>;
};

template <class... A>
struct nowned<tuple<A...>> {
    using type = tuple<typename nowned<remove_cvref_t<A>>::type...>;
};

} // namespace ndetail

template <class T>
using nowned_t = typename ndetail::nowned<remove_cvref_t<T>>::type;

template <class T>
constexpr nowned_t<T> nown(T&& value) {
    return nowned_t<T>(forward<T>(value));
}

namespace ndetail {

template <class A>
struct nref {
    A* source;

    constexpr decltype(auto) operator()(nidx_t i) const { return (*source)[i]; }

    template <class K>
    requires requires(A& a, K&& key) { a.inverse(forward<K>(key)); }
    constexpr decltype(auto) inverse(K&& key) const {
        return source->inverse(forward<K>(key));
    }
};

template <class V>
struct ninverse_ref {
    V* view;

    template <class K>
    constexpr decltype(auto) operator()(K&& key) const {
        return view->inverse(forward<K>(key));
    }
};

} // namespace ndetail

template <class V>
requires requires(V& view) { view.inverse(view[0]); }
constexpr auto nlocate(V& view) {
    return ndetail::ninverse_ref<V>{addressof(view)};
}

template <class A>
constexpr auto nall(A& source) {
    return nview{nlen(source), ndetail::nref<A>{addressof(source)}};
}

template <class N, class F>
requires (!nidx_wider_v<N>)
constexpr auto ntabulate(N length, F f) {
    return nview{nidx_t(length), move(f)};
}

template <class N, class F>
requires nidx_wider_v<N>
constexpr auto ntabulate(N, F) = delete;

namespace ndetail {

template <class F, class I>
struct ninvertible {
    [[no_unique_address]] F forward_map;
    [[no_unique_address]] I backward_map;

    constexpr decltype(auto) operator()(nidx_t i) {
        return invoke(forward_map, i);
    }

    template <class K>
    constexpr decltype(auto) inverse(K&& key) {
        return invoke(backward_map, forward<K>(key));
    }
};

} // namespace ndetail

template <class N, class F, class I>
requires (!nidx_wider_v<N>)
constexpr auto ntabulate(N length, F forward_map, I backward_map) {
    return nview{nidx_t(length),
                 ndetail::ninvertible<F, I>{move(forward_map), move(backward_map)}};
}

template <class N, class F, class I>
requires nidx_wider_v<N>
constexpr auto ntabulate(N, F, I) = delete;

namespace ndetail {

struct nrange_access {
    nidx_t first;

    constexpr nidx_t operator()(nidx_t i) const { return first + i; }
    constexpr nidx_t inverse(nidx_t key) const { return key - first; }
};

} // namespace ndetail

template <class A, class B>
requires (!nidx_wider_v<A> && !nidx_wider_v<B>)
constexpr auto nrange(A first, B last) {
    return nview{nidx_t(last) - nidx_t(first), ndetail::nrange_access{nidx_t(first)}};
}

template <class A, class B>
requires (nidx_wider_v<A> || nidx_wider_v<B>)
constexpr auto nrange(A, B) = delete;

template <class N>
requires (!nidx_wider_v<N>)
constexpr auto nrange(N length) {
    return nrange(nidx_t(0), length);
}

template <class N>
requires nidx_wider_v<N>
constexpr auto nrange(N) = delete;

namespace ndetail {

template <class V>
struct nsub_access {
    V view;
    nidx_t first;

    constexpr decltype(auto) operator()(nidx_t i) { return view[first + i]; }

    template <class K>
    requires requires(V& source, K&& key) { source.inverse(forward<K>(key)); }
    constexpr nidx_t inverse(K&& key) {
        return view.inverse(forward<K>(key)) - first;
    }
};

} // namespace ndetail

template <class V, class A, class B>
requires (!nidx_wider_v<A> && !nidx_wider_v<B>)
constexpr auto nsub(V view, A first, B last) {
    return nview{nidx_t(last) - nidx_t(first),
                 ndetail::nsub_access<V>{move(view), nidx_t(first)}};
}

template <class V, class A, class B>
requires (nidx_wider_v<A> || nidx_wider_v<B>)
constexpr auto nsub(V, A, B) = delete;

namespace ndetail {

template <class V>
struct nreverse_access {
    V view;
    nidx_t length;

    constexpr decltype(auto) operator()(nidx_t i) {
        return view[length - 1 - i];
    }

    template <class K>
    requires requires(V& source, K&& key) { source.inverse(forward<K>(key)); }
    constexpr nidx_t inverse(K&& key) {
        return length - 1 - view.inverse(forward<K>(key));
    }
};

} // namespace ndetail

template <class V>
constexpr auto nreverse(V view) {
    nidx_t n = view.len();
    return nview{n, ndetail::nreverse_access<V>{move(view), n}};
}

/* nproject keeps the callable's result category; nmap deliberately materializes it. */
template <class V, class F>
constexpr auto nproject(V view, F f) {
    return nview{view.len(),
                 [view = move(view), f = move(f)](nidx_t i) mutable -> decltype(auto) {
                     return invoke(f, view[i]);
                 }};
}

template <class V, class F>
constexpr auto nmap(V view, F f) {
    return nview{view.len(),
                 [view = move(view), f = move(f)](nidx_t i) mutable {
                     return invoke(f, view[i]);
                 }};
}

namespace ndetail {

template <class V, class I>
struct ngather_access {
    V view;
    I positions;

    constexpr decltype(auto) operator()(nidx_t i) {
        return view[positions[i]];
    }

    template <class K>
    requires requires(V& source, I& plan, K&& key) {
        source.inverse(forward<K>(key));
        plan.inverse(source.inverse(forward<K>(key)));
    }
    constexpr nidx_t inverse(K&& key) {
        return positions.inverse(view.inverse(forward<K>(key)));
    }
};

} // namespace ndetail

template <class V, class I>
constexpr auto ngather(V view, I positions) {
    return nview{positions.len(), ndetail::ngather_access<V, I>{move(view), move(positions)}};
}

/* Zip stops at the shortest input and returns a tuple of the input result categories. */
template <class V, class... W>
constexpr auto nzip(V first, W... rest) {
    auto views = tuple<V, W...>(move(first), move(rest)...);
    nidx_t n = apply([](const auto&... x) { return min({x.len()...}); }, views);
    return nview{n, [views = move(views)](nidx_t i) mutable {
                     return apply([&](auto&... x) {
                         return tuple<decltype(x[i])...>(x[i]...);
                     }, views);
                 }};
}

namespace ndetail {

template <class Tuple, size_t... I>
constexpr auto nproduct_lengths(const Tuple& views, index_sequence<I...>) {
    return array<nidx_t, sizeof...(I)>{get<I>(views).len()...};
}

template <class Tuple, size_t... I>
constexpr auto nproduct_at(Tuple& views, const array<nidx_t, sizeof...(I)>& lengths,
                           nidx_t flat, index_sequence<I...>) {
    array<nidx_t, sizeof...(I)> position{};
    nidx_t dimension = nidx_t(sizeof...(I));
    while (dimension) {
        --dimension;
        position[dimension] = flat % lengths[dimension];
        flat /= lengths[dimension];
    }
    return tuple<decltype(get<I>(views)[position[I]])...>(
        get<I>(views)[position[I]]...
    );
}

template <class X, class Y>
struct nproduct_pair {
    X left;
    Y right;
    nidx_t width;

    constexpr auto operator()(nidx_t flat) {
        nidx_t x = flat / width, y = flat % width;
        return pair<decltype(left[x]), decltype(right[y])>(left[x], right[y]);
    }

    template <class K>
    requires requires(X& x, Y& y, K&& key) {
        x.inverse(key.first);
        y.inverse(key.second);
    }
    constexpr nidx_t inverse(K&& key) {
        return left.inverse(key.first) * width + right.inverse(key.second);
    }
};

template <class Tuple, class Key, size_t... I>
constexpr bool nproduct_has_inverse(index_sequence<I...>) {
    return (requires(Tuple& views, const Key& key) {
        get<I>(views).inverse(get<I>(key));
    } && ...);
}

template <class Tuple, class Key, size_t... I>
constexpr nidx_t nproduct_inverse(Tuple& views,
                                  const array<nidx_t, sizeof...(I)>& lengths,
                                  const Key& key, index_sequence<I...>) {
    nidx_t flat = 0;
    ((flat = flat * lengths[I] + get<I>(views).inverse(get<I>(key))), ...);
    return flat;
}

template <class Tuple, size_t N>
struct nproduct_tuple {
    Tuple views;
    array<nidx_t, N> lengths;

    constexpr auto operator()(nidx_t flat) {
        return nproduct_at(views, lengths, flat, make_index_sequence<N>{});
    }

    template <class K>
    requires (nproduct_has_inverse<Tuple, K>(make_index_sequence<N>{}))
    constexpr nidx_t inverse(const K& key) {
        return nproduct_inverse(views, lengths, key, make_index_sequence<N>{});
    }
};

} // namespace ndetail

/* Left-major product; the last axis changes fastest.  The product must fit nidx_t. */
template <class X, class Y>
constexpr auto nproduct(X left, Y right) {
    nidx_t width = right.len();
    nidx_t length = nidx_t(__int128_t(left.len()) * right.len());
    return nview{length, ndetail::nproduct_pair<X, Y>{move(left), move(right), width}};
}

template <class X, class Y, class... Z>
requires (sizeof...(Z) > 0)
constexpr auto nproduct(X first, Y second, Z... rest) {
    constexpr size_t dimensions = 2 + sizeof...(Z);
    auto views = tuple<X, Y, Z...>(move(first), move(second), move(rest)...);
    auto lengths = ndetail::nproduct_lengths(views, make_index_sequence<dimensions>{});
    __int128_t total = 1;
    for (nidx_t length : lengths) total *= length;
    return nview{
        nidx_t(total),
        ndetail::nproduct_tuple<decltype(views), dimensions>{move(views), lengths}
    };
}
