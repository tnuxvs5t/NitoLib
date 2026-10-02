#pragma once
#include <algorithm>
#include <bit>
#include <utility>
#include <vector>

// Algebra: value_type/tag_type, id(), join(a,b), apply(value,tag,length),
// compose(newer,older). join is an ordered monoid; tags act distributively and
// compose in chronological order (older first). No tag identity is required:
// pending bits separate a default-constructed tag from a meaningful tag.
// Uniform actions only; Beats/position-dependent propagation stay separate.
// Valid [l,r) within [0,n). Build O(n); update/query/set O(log n), scalar algebra.
// n and 2*base fit int; values and tags copyable, tags default-constructible.
template <class Algebra>
struct lazy_segtree {
    using T = typename Algebra::value_type;
    using F = typename Algebra::tag_type;
    int n, base;
    [[no_unique_address]] Algebra op;
    std::vector<T> data;
    std::vector<F> lazy;
    std::vector<unsigned char> pending;

    template <class R>
    explicit lazy_segtree(const R& source, Algebra operation = {})
        : n(int(source.size())), base(int(std::bit_ceil(unsigned(std::max(1, n))))),
          op(std::move(operation)), data(2 * base, op.id()), lazy(base), pending(base) {
        for (int i = 0; i < n; ++i) data[base + i] = source[i];
        for (int i = base - 1; i; --i) pull(i);
    }
    void pull(int v) { data[v] = op.join(data[2 * v], data[2 * v + 1]); }
    void put(int v, int length, const F& tag) {
        data[v] = op.apply(data[v], tag, length);
        if (v < base) {
            lazy[v] = pending[v] ? op.compose(tag, lazy[v]) : tag;
            pending[v] = true;
        }
    }
    void push(int v, int length) {
        if (v >= base || !pending[v]) return;
        put(2 * v, length / 2, lazy[v]);
        put(2 * v + 1, length / 2, lazy[v]);
        pending[v] = false;
    }
    void apply(int left, int right, const F& tag) {
        auto visit = [&](auto&& self, int v, int lo, int hi) -> void {
            if (right <= lo || hi <= left) return;
            if (left <= lo && hi <= right) { put(v, hi - lo, tag); return; }
            push(v, hi - lo);
            int mid = (lo + hi) / 2;
            self(self, 2 * v, lo, mid);
            self(self, 2 * v + 1, mid, hi);
            pull(v);
        };
        if (left != right) visit(visit, 1, 0, base);
    }
    T fold(int left, int right) {
        auto visit = [&](auto&& self, int v, int lo, int hi) -> T {
            if (right <= lo || hi <= left) return op.id();
            if (left <= lo && hi <= right) return data[v];
            push(v, hi - lo);
            int mid = (lo + hi) / 2;
            T a = self(self, 2 * v, lo, mid);
            T b = self(self, 2 * v + 1, mid, hi);
            return op.join(a, b);
        };
        return left == right ? op.id() : visit(visit, 1, 0, base);
    }
    void set(int position, T value) {
        int v = 1, lo = 0, hi = base;
        while (v < base) {
            push(v, hi - lo);
            int mid = (lo + hi) / 2;
            if (position < mid) { v *= 2; hi = mid; }
            else { v = 2 * v + 1; lo = mid; }
        }
        data[v] = std::move(value);
        while (v >>= 1) pull(v);
    }
    const T& all() const { return data[1]; }
};
