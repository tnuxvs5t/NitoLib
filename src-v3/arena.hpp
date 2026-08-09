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

    int len() const { return int(data.size()); }
    void reserve(int n) { data.reserve(n); }
    T& operator[](int handle) { return data[handle]; }
    const T& operator[](int handle) const { return data[handle]; }

    template <class... A>
    int make(A&&... args) {
        data.emplace_back(forward<A>(args)...);
        return int(data.size()) - 1;
    }
};
