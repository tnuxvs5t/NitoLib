#pragma once
#include "core.hpp"

/*
An nview is only a finite positional projection: position i calls access(i).
It owns its accessor, not the referenced data.  Const is deliberately shallow: a
const descriptor may still yield T&.  Public accessors must not return references to
call-local objects or T&&.  Structural validity and [0,len()) are caller contracts.
*/
template <class A>
struct nview {
    int length;
    mutable A access;

    constexpr int len() const { return length; }
    constexpr bool empty() const { return !length; }
    constexpr decltype(auto) operator[](int i) const { return invoke(access, i); }

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

/* nall borrows an lvalue.  There is deliberately no temporary-owner overload. */
template <class A>
constexpr auto nall(A& a) {
    return nview{nlen(a), [p = addressof(a)](int i) -> decltype(auto) { return (*p)[i]; }};
}

template <class F>
constexpr auto ntabulate(int n, F f) {
    return nview{n, move(f)};
}

constexpr auto nrange(int first, int last) {
    return ntabulate(last - first, [first](int i) { return first + i; });
}

constexpr auto nrange(int n) { return nrange(0, n); }

template <class V>
constexpr auto nsub(V view, int first, int last) {
    return nview{last - first,
                 [view = move(view), first](int i) mutable -> decltype(auto) {
                     return view[first + i];
                 }};
}

template <class V>
constexpr auto nreverse(V view) {
    int n = view.len();
    return nview{n, [view = move(view), n](int i) mutable -> decltype(auto) {
                     return view[n - 1 - i];
                 }};
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
    return nview{n, [view = move(view), positions = move(positions)](int i) mutable
                        -> decltype(auto) { return view[positions[i]]; }};
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

/* Left-major Cartesian product.  The product length must fit signed int. */
template <class X, class Y>
constexpr auto nproduct(X left, Y right) {
    int n = int(1LL * left.len() * right.len());
    return nview{n, [left = move(left), right = move(right)](int i) mutable {
                     int width = right.len(), x = i / width, y = i % width;
                     return pair<decltype(left[x]), decltype(right[y])>(left[x], right[y]);
                 }};
}
