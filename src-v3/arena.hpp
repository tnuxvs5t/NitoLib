#pragma once
#include "core.hpp"

/*
Append-only indexed storage.  Handles survive vector relocation; references do not.
The arena deliberately has no generation, epoch, owner or automatic reclamation.
Copying an arena copies every node.  A structure that wants recycling may layer it on
top, with its own stale-handle contract.
*/
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
