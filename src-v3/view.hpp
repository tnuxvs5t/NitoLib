#pragma once
#include "core.hpp"

/*
An nview is only a finite positional projection: position i calls access(i).
It owns its accessor, not the referenced data.  Const is deliberately shallow: a
const descriptor may still yield T&.  Public accessors must not return references to
call-local objects or T&&.  An accessor may additionally expose inverse(key), meaning
the projection is injective and inverse((*this)[i]) == i.  Structural validity,
inverse membership and [0,len()) are caller contracts.
*/
template <class A>
struct nview {
    int length;
    mutable A access;

    constexpr int len() const { return length; }
    constexpr bool empty() const { return !length; }
    constexpr decltype(auto) operator[](int i) const { return invoke(access, i); }

    template <class K>
    requires requires(A& accessor, K&& key) {
        accessor.inverse(forward<K>(key));
    }
    constexpr decltype(auto) inverse(K&& key) const {
        return access.inverse(forward<K>(key));
    }

    struct iterator {
        using difference_type = int;
        using reference = decltype(declval<const nview&>()[0]);
        using value_type = remove_cvref_t<reference>;
        using pointer = void;
        using iterator_category = random_access_iterator_tag;
        using iterator_concept = random_access_iterator_tag;

        const nview* view = nullptr;
        int position = 0;

        constexpr decltype(auto) operator*() const { return (*view)[position]; }
        constexpr decltype(auto) operator[](int d) const { return (*view)[position + d]; }
        constexpr iterator& operator++() { ++position; return *this; }
        constexpr iterator operator++(int) { auto old = *this; ++*this; return old; }
        constexpr iterator& operator--() { --position; return *this; }
        constexpr iterator operator--(int) { auto old = *this; --*this; return old; }
        constexpr iterator& operator+=(int d) { position += d; return *this; }
        constexpr iterator& operator-=(int d) { position -= d; return *this; }
        friend constexpr iterator operator+(iterator it, int d) { return it += d; }
        friend constexpr iterator operator+(int d, iterator it) { return it += d; }
        friend constexpr iterator operator-(iterator it, int d) { return it -= d; }
        constexpr int operator-(iterator other) const { return position - other.position; }
        constexpr auto operator<=>(const iterator&) const = default;
    };

    constexpr iterator begin() const { return {this, 0}; }
    constexpr iterator end() const { return {this, length}; }
};

template <class A>
nview(int, A) -> nview<A>;

namespace nview_detail {
template <class T>
struct owned { using type = remove_cvref_t<T>; };

template <class A, class B>
struct owned<pair<A, B>> {
    using type = pair<typename owned<remove_cvref_t<A>>::type,
                      typename owned<remove_cvref_t<B>>::type>;
};

template <class... T>
struct owned<tuple<T...>> {
    using type = tuple<typename owned<remove_cvref_t<T>>::type...>;
};

template <class T>
using owned_t = typename owned<remove_cvref_t<T>>::type;

template <class T>
constexpr auto own(T&& value) -> owned_t<T> {
    return owned_t<T>(forward<T>(value));
}

template <class V>
struct inverse_ref {
    V* view;

    template <class K>
    constexpr decltype(auto) operator()(K&& key) const {
        return view->inverse(forward<K>(key));
    }
};
}

/* nlocate borrows an invertible lvalue view; the returned callable never extends its life. */
template <class V>
requires requires(V& view) { view.inverse(view[0]); }
constexpr auto nlocate(V& view) {
    return nview_detail::inverse_ref<V>{addressof(view)};
}

/* nall borrows an lvalue.  There is deliberately no temporary-owner overload. */
template <class A>
constexpr auto nall(A& a) {
    return nview{nlen(a), [p = addressof(a)](int i) -> decltype(auto) { return (*p)[i]; }};
}

template <class F>
constexpr auto ntabulate(int n, F f) {
    return nview{n, move(f)};
}

namespace nview_detail {
template <class F, class I>
struct invertible_access {
    [[no_unique_address]] F project;
    [[no_unique_address]] I backward;

    constexpr decltype(auto) operator()(int position) {
        return invoke(project, position);
    }

    template <class K>
    constexpr decltype(auto) inverse(K&& key) {
        return invoke(backward, forward<K>(key));
    }
};

struct range_access {
    int first;

    constexpr int operator()(int position) const { return first + position; }
    constexpr int inverse(int key) const { return key - first; }
};

template <class V>
struct sub_access {
    V view;
    int first;

    constexpr decltype(auto) operator()(int position) { return view[first + position]; }

    template <class K>
    requires requires(V& source, K&& key) {
        source.inverse(forward<K>(key));
    }
    constexpr int inverse(K&& key) {
        return view.inverse(forward<K>(key)) - first;
    }
};

template <class V>
struct reverse_access {
    V view;
    int length;

    constexpr decltype(auto) operator()(int position) {
        return view[length - 1 - position];
    }

    template <class K>
    requires requires(V& source, K&& key) {
        source.inverse(forward<K>(key));
    }
    constexpr int inverse(K&& key) {
        return length - 1 - view.inverse(forward<K>(key));
    }
};

template <class V, class I>
struct gather_access {
    V view;
    I positions;

    constexpr decltype(auto) operator()(int position) {
        return view[positions[position]];
    }

    template <class K>
    requires requires(V& source, I& plan, K&& key) {
        plan.inverse(source.inverse(forward<K>(key)));
    }
    constexpr int inverse(K&& key) {
        return positions.inverse(view.inverse(forward<K>(key)));
    }
};
}

template <class F, class I>
constexpr auto ntabulate(int n, F forward, I inverse) {
    return nview{n, nview_detail::invertible_access<F, I>{move(forward), move(inverse)}};
}

constexpr auto nrange(int first, int last) {
    return nview{last - first, nview_detail::range_access{first}};
}

constexpr auto nrange(int n) { return nrange(0, n); }

template <class V>
constexpr auto nsub(V view, int first, int last) {
    return nview{last - first, nview_detail::sub_access<V>{move(view), first}};
}

template <class V>
constexpr auto nreverse(V view) {
    int n = view.len();
    return nview{n, nview_detail::reverse_access<V>{move(view), n}};
}

/* nproject preserves the callable's exact result category; nmap materializes it. */
template <class V, class F>
constexpr auto nproject(V view, F f) {
    int n = view.len();
    return nview{n, [view = move(view), f = move(f)](int i) mutable -> decltype(auto) {
                     return invoke(f, view[i]);
                 }};
}

template <class V, class F>
constexpr auto nmap(V view, F f) {
    int n = view.len();
    return nview{n, [view = move(view), f = move(f)](int i) mutable {
                     return invoke(f, view[i]);
                 }};
}

template <class V, class I>
constexpr auto ngather(V view, I positions) {
    int n = positions.len();
    return nview{n, nview_detail::gather_access<V, I>{move(view), move(positions)}};
}

/* Zip stops at the shortest input.  Its tuple elements retain references. */
template <class V, class... W>
constexpr auto nzip(V first, W... rest) {
    auto views = tuple<V, W...>(move(first), move(rest)...);
    int n = apply([](const auto&... x) { return min({x.len()...}); }, views);
    return nview{n, [views = move(views)](int i) mutable {
                     return apply([&](auto&... x) {
                         return tuple<decltype(x[i])...>(x[i]...);
                     }, views);
                 }};
}

namespace nproduct_detail {
template <class Tuple, size_t... I>
constexpr auto lengths(const Tuple& views, index_sequence<I...>) {
    return array<int, sizeof...(I)>{get<I>(views).len()...};
}

template <class Tuple, size_t... I>
constexpr auto tuple_at(Tuple& views, const array<int, sizeof...(I)>& lengths,
                        int flat, index_sequence<I...>) {
    array<int, sizeof...(I)> position{};
    for (int dimension = int(sizeof...(I)) - 1; dimension >= 0; --dimension) {
        position[dimension] = flat % lengths[dimension];
        flat /= lengths[dimension];
    }
    return tuple<decltype(get<I>(views)[position[I]])...>(
        get<I>(views)[position[I]]...
    );
}

template <class X, class Y>
struct pair_access {
    X left;
    Y right;
    int width;

    constexpr auto operator()(int flat) {
        int x = flat / width, y = flat % width;
        return pair<decltype(left[x]), decltype(right[y])>(left[x], right[y]);
    }

    template <class K>
    requires requires(X& x, Y& y, K&& key) {
        x.inverse(key.first);
        y.inverse(key.second);
    }
    constexpr int inverse(K&& key) {
        return left.inverse(key.first) * width + right.inverse(key.second);
    }
};

template <class Tuple, class Key, size_t... I>
constexpr bool inverse_available(index_sequence<I...>) {
    return (requires(Tuple& views, const Key& key) {
        get<I>(views).inverse(get<I>(key));
    } && ...);
}

template <class Tuple, class Key, size_t... I>
constexpr int tuple_inverse(Tuple& views, const array<int, sizeof...(I)>& lengths,
                            const Key& key, index_sequence<I...>) {
    int flat = 0;
    ((flat = flat * lengths[I] + get<I>(views).inverse(get<I>(key))), ...);
    return flat;
}

template <class Tuple, size_t N>
struct tuple_access {
    Tuple views;
    array<int, N> lengths;

    constexpr auto operator()(int flat) {
        return tuple_at(views, lengths, flat, make_index_sequence<N>{});
    }

    template <class K>
    requires (inverse_available<Tuple, K>(make_index_sequence<N>{}))
    constexpr int inverse(const K& key) {
        return tuple_inverse(views, lengths, key, make_index_sequence<N>{});
    }
};
}

/* Left-major Cartesian product.  The product length must fit signed int. */
template <class X, class Y>
constexpr auto nproduct(X left, Y right) {
    int n = int(1LL * left.len() * right.len());
    int width = right.len();
    return nview{n, nproduct_detail::pair_access<X, Y>{move(left), move(right), width}};
}

/* Three or more axes return a tuple view; the last axis changes fastest. */
template <class X, class Y, class... Z>
requires (sizeof...(Z) > 0)
constexpr auto nproduct(X first, Y second, Z... rest) {
    constexpr size_t dimensions = 2 + sizeof...(Z);
    auto views = tuple<X, Y, Z...>(move(first), move(second), move(rest)...);
    auto lengths = nproduct_detail::lengths(
        views, make_index_sequence<dimensions>{}
    );
    long long total = 1;
    for (int length : lengths)
        total *= length;
    return nview{
        int(total),
        nproduct_detail::tuple_access<decltype(views), dimensions>{move(views), lengths}
    };
}
