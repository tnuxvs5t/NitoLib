#pragma once
#include "arena.hpp"

/* A pooled doubly-linked chain kernel.  Roots alias node sets; handles survive
   arena growth, while payload references do not.  erase recycles the slot. */
template <class T>
struct nlist {
    struct node {
        nidx_t prev = -1, next = -1;
        optional<T> value;

        template <class... A>
        explicit node(in_place_t, A&&... args)
            : value(in_place, forward<A>(args)...) {}
    };

    struct root {
        nidx_t first = -1, last = -1;
        bool empty() const { return first < 0; }
    };

    narena<node> pool;
    nidx_t free = -1;

    void reserve(nidx_t count) { pool.reserve(count); }

    T& operator[](nidx_t handle) { return *pool[handle].value; }
    const T& operator[](nidx_t handle) const { return *pool[handle].value; }
    nidx_t prev(nidx_t handle) const { return pool[handle].prev; }
    nidx_t next(nidx_t handle) const { return pool[handle].next; }

    void join(nidx_t left, nidx_t right) {
        if (left >= 0) pool[left].next = right;
        if (right >= 0) pool[right].prev = left;
    }

    root cut(root& chain, nidx_t first, nidx_t last) {
        if (first == last) return {};
        nidx_t before = prev(first);
        nidx_t tail = last < 0 ? chain.last : prev(last);
        join(before, last);
        if (before < 0) chain.first = last;
        if (last < 0) chain.last = before;
        pool[first].prev = -1;
        pool[tail].next = -1;
        return {first, tail};
    }

    void splice(root& chain, nidx_t position, root& part) {
        if (part.empty()) return;
        nidx_t before = position < 0 ? chain.last : prev(position);
        join(before, part.first);
        join(part.last, position);
        if (before < 0) chain.first = part.first;
        if (position < 0) chain.last = part.last;
        part = {};
    }

    template <class... A>
    nidx_t insert(root& chain, nidx_t position, A&&... args) {
        nidx_t handle = free;
        if (handle < 0) {
            handle = pool.make(in_place, forward<A>(args)...);
        } else {
            pool[handle].value.emplace(forward<A>(args)...);
            free = pool[handle].next;
        }
        root part{handle, handle};
        splice(chain, position, part);
        return handle;
    }

    nidx_t erase(root& chain, nidx_t handle) {
        nidx_t after = next(handle);
        cut(chain, handle, after);
        pool[handle].value.reset();
        pool[handle].next = free;
        free = handle;
        return after;
    }

    void clear(root& chain) {
        while (!chain.empty()) erase(chain, chain.first);
    }

    void reverse(root& chain) {
        for (nidx_t handle = chain.first; handle >= 0; handle = prev(handle))
            swap(pool[handle].prev, pool[handle].next);
        swap(chain.first, chain.last);
    }
};
