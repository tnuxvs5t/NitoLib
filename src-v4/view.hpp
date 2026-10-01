#pragma once
#include "core.hpp"

template <class A>
struct nref {
    A* p;

    decltype(auto) operator()(nidx_t i) const { return (*p)[i]; }

    template <class K>
    requires requires(A& a, K&& key) { a.inverse(forward<K>(key)); }
    decltype(auto) inverse(K&& key) const {
        return p->inverse(forward<K>(key));
    }
};

template <class A>
struct nview {
    nidx_t length;
    mutable A access;

    nidx_t len() const { return length; }
    bool empty() const { return !length; }
    decltype(auto) operator[](nidx_t i) const { return invoke(access, i); }

    struct iterator {
        using difference_type = nidx_t;
        using value_type = remove_cvref_t<decltype(declval<const nview&>()[0])>;
        using reference = decltype(declval<const nview&>()[0]);
        using pointer = void;
        using iterator_category = random_access_iterator_tag;
        using iterator_concept = random_access_iterator_tag;

        const nview* view = nullptr;
        nidx_t position = 0;
        decltype(auto) operator*() const { return (*view)[position]; }
        decltype(auto) operator[](nidx_t d) const { return (*view)[position + d]; }
        iterator& operator++() { ++position; return *this; }
        iterator operator++(int) { auto old = *this; ++*this; return old; }
        iterator& operator--() { --position; return *this; }
        iterator operator--(int) { auto old = *this; --*this; return old; }
        iterator& operator+=(nidx_t d) { position += d; return *this; }
        iterator& operator-=(nidx_t d) { position -= d; return *this; }
        friend iterator operator+(iterator it, nidx_t d) { return it += d; }
        friend iterator operator+(nidx_t d, iterator it) { return it += d; }
        friend iterator operator-(iterator it, nidx_t d) { return it -= d; }
        friend nidx_t operator-(iterator a, iterator b) { return a.position - b.position; }
        friend bool operator==(iterator a, iterator b) {
            return a.view == b.view && a.position == b.position;
        }
        friend auto operator<=>(iterator a, iterator b) { return a.position <=> b.position; }
    };

    iterator begin() const { return {this, 0}; }
    iterator end() const { return {this, length}; }

    template <class K>
    requires requires(A& a, K&& key) { a.inverse(forward<K>(key)); }
    decltype(auto) inverse(K&& key) const {
        return access.inverse(forward<K>(key));
    }
};

template <class A>
nview(nidx_t, A) -> nview<A>;

template <class A>
auto nall(A& a) {
    return nview{nlen(a), nref<A>{addressof(a)}};
}

template <class A>
auto nall(const A& a) {
    return nview{nlen(a), nref<const A>{addressof(a)}};
}

constexpr auto nrange(nidx_t n) {
    return nview{n, [=](nidx_t i) { return i; }};
}

template <class V>
auto nsub(V view, nidx_t left, nidx_t right) {
    return nview{right - left,
                 [view = move(view), left](nidx_t i) mutable -> decltype(auto) {
                     return view[left + i];
                 }};
}

template <class V, class F>
auto nproject(V view, F f) {
    return nview{view.len(),
                 [view = move(view), f = move(f)](nidx_t i) mutable -> decltype(auto) {
                     return invoke(f, view[i]);
                 }};
}

template <class V, class F>
auto nmap(V view, F f) {
    return nview{view.len(),
                 [view = move(view), f = move(f)](nidx_t i) mutable {
                     return invoke(f, view[i]);
                 }};
}

template <class V>
requires requires(V& view, decltype(view[0]) key) { view.inverse(key); }
auto nlocate(V& view) {
    return [p = addressof(view)](auto&& key) -> decltype(auto) {
        return p->inverse(forward<decltype(key)>(key));
    };
}
