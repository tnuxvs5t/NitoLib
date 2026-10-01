#pragma once
#include "core.hpp"

/* Append-only handles: vector relocation preserves handles, not references. */
template <class T>
struct narena {
    vector<T> data;

    nidx_t len() const { return nidx_t(data.size()); }
    void reserve(nidx_t n) { data.reserve(n); }
    T& operator[](nidx_t handle) { return data[handle]; }
    const T& operator[](nidx_t handle) const { return data[handle]; }

    template <class... A>
    nidx_t make(A&&... args) {
        data.emplace_back(forward<A>(args)...);
        return nidx_t(data.size()) - 1;
    }
};
