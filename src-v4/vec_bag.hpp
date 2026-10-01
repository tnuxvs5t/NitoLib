#pragma once
#include "discrete.hpp"

/* Contiguous ordered multiset baseline.  Positions and sequence views are
   invalidated by structural mutation; insertion and erasure are linear. */
template <class T, class C = less<>>
struct nvec_bag {
private:
    vector<T> values;
    [[no_unique_address]] mutable C compare;

    auto readonly() const { return nall(as_const(values)); }

public:
    explicit nvec_bag(C order = {}) : compare(move(order)) {}

    template <class V>
    requires requires(V& source) { nlen(source); source[0]; }
    explicit nvec_bag(const V& source, C order = {}) : nvec_bag(move(order)) {
        values.reserve(nlen(source));
        for (nidx_t i = 0; i < nlen(source); ++i)
            values.emplace_back(T(nown(source[i])));
        stable_sort(values.begin(), values.end(), ref(compare));
    }

    nidx_t len() const { return nidx_t(values.size()); }
    bool empty() const { return values.empty(); }
    void reserve(nidx_t count) { values.reserve(count); }

    nidx_t lower_bound(const T& key) const {
        return nlower(readonly(), key, ref(compare));
    }

    nidx_t upper_bound(const T& key) const {
        return nupper(readonly(), key, ref(compare));
    }

    template <class K, class D, class P = identity>
    nidx_t lower_bound(const K& key, D order, P projection = {}) const {
        return nlower(readonly(), key, move(order), move(projection));
    }

    template <class K, class D, class P = identity>
    nidx_t upper_bound(const K& key, D order, P projection = {}) const {
        return nupper(readonly(), key, move(order), move(projection));
    }

    nidx_t insert(T value) {
        nidx_t position = upper_bound(value);
        values.insert(values.begin() + position, move(value));
        return position;
    }

    template <class... A>
    nidx_t emplace(A&&... args) { return insert(T(forward<A>(args)...)); }

    pair<nidx_t, nidx_t> equal_range(const T& key) const {
        return {lower_bound(key), upper_bound(key)};
    }

    nidx_t count(const T& key) const {
        auto [left, right] = equal_range(key);
        return right - left;
    }

    nidx_t find(const T& key) const {
        nidx_t position = lower_bound(key);
        return position < len() && !invoke(compare, key, values[position])
                   ? position : len();
    }

    bool contains(const T& key) const { return find(key) != len(); }

    bool erase_one(const T& key) {
        nidx_t position = lower_bound(key);
        if (position == len() || invoke(compare, key, values[position])) return false;
        values.erase(values.begin() + position);
        return true;
    }

    nidx_t erase_all(const T& key) {
        auto [left, right] = equal_range(key);
        values.erase(values.begin() + left, values.begin() + right);
        return right - left;
    }

    T erase_at(nidx_t position) {
        T value = move(values[position]);
        values.erase(values.begin() + position);
        return value;
    }

    void clear() { values.clear(); }

    decltype(auto) kth(nidx_t position) const { return values[position]; }
    decltype(auto) operator[](nidx_t position) const { return kth(position); }
    decltype(auto) front() const { return kth(0); }
    decltype(auto) back() const { return kth(len() - 1); }

    auto sequence() const { return readonly(); }
};
