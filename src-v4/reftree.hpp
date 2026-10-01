#pragma once
#include "arena.hpp"

struct nreftree_noop {
    monostate join(const auto&, nidx_t, nidx_t, nidx_t) const { return {}; }
};

/* Persistent binary patterns.  A root repeats its stored pattern at any taller
   observation height; join only reduces equal handles, never equal values. */
template <class Info = monostate, class Ops = nreftree_noop>
class nreftree {
public:
    using U = uint64_t;

    struct node {
        [[no_unique_address]] Info value;
        nidx_t left, right, height;
    };

private:
    narena<node> pool;

public:
    [[no_unique_address]] Ops ops;

    explicit nreftree(Ops policy = {}) : ops(move(policy)) {}

    const node& operator[](nidx_t handle) const { return pool[handle]; }
    nidx_t nodes() const { return pool.len(); }
    void reserve(nidx_t count) { pool.reserve(count); }

    template <class... A>
    nidx_t leaf(A&&... args) {
        return pool.make(node{Info(forward<A>(args)...), -1, -1, 0});
    }

    pair<nidx_t, nidx_t> split(nidx_t handle, nidx_t height) const {
        if (pool[handle].height == height)
            return {pool[handle].left, pool[handle].right};
        return {handle, handle};
    }

    nidx_t join(nidx_t height, nidx_t left, nidx_t right) {
        if (left == right) return left;
        Info value = ops.join(as_const(*this), height, left, right);
        return pool.make(node{move(value), left, right, height});
    }

    nidx_t leaf_at(nidx_t handle, U x) const {
        while (pool[handle].height) {
            const auto& item = pool[handle];
            handle = (x >> (item.height - 1) & 1) ? item.right : item.left;
        }
        return handle;
    }

    nidx_t block(nidx_t handle, U x, nidx_t height) const {
        while (pool[handle].height > height) {
            const auto& item = pool[handle];
            handle = (x >> (item.height - 1) & 1) ? item.right : item.left;
        }
        return handle;
    }

    nidx_t paste(nidx_t handle, nidx_t height, U x, nidx_t width,
                 nidx_t source) {
        if (height == width || handle == source) return source;
        auto [left, right] = split(handle, height);
        U half = U(1) << (height - 1);
        if (x < half) left = paste(left, height - 1, x, width, source);
        else right = paste(right, height - 1, x - half, width, source);
        return join(height, left, right);
    }

    nidx_t set(nidx_t handle, nidx_t height, U x, nidx_t terminal) {
        return paste(handle, height, x, 0, terminal);
    }

    template <class P>
    U find_first(nidx_t handle, nidx_t height, U x, P&& has) const {
        auto search = [&](auto&& self, nidx_t root, nidx_t h, U start) -> U {
            U end = U(1) << h;
            if (start >= end || !invoke(has, root, h)) return end;
            if (!h) return 0;
            auto [left, right] = split(root, h);
            U half = end >> 1;
            if (start < half) {
                U answer = self(self, left, h - 1, start);
                if (answer < half) return answer;
                start = 0;
            } else {
                start -= half;
            }
            return half + self(self, right, h - 1, start);
        };
        return search(search, handle, height, x);
    }

    template <class Count>
    U kth(nidx_t handle, nidx_t height, U position, Count&& count) const {
        U offset = 0;
        while (height) {
            auto [left, right] = split(handle, height);
            U amount = invoke(count, left, height - 1);
            --height;
            if (position < amount) {
                handle = left;
            } else {
                position -= amount;
                offset += U(1) << height;
                handle = right;
            }
        }
        return offset;
    }
};
