#pragma once
#include "arena.hpp"

/*
One pool, any number of disjoint doubly linked chains. A root is only {first,last};
-1 denotes a missing neighbor / insertion at the end, NOT a positional index.
operator[](h) is the payload; prev/next and pool expose the actual topology.
Root copies alias: cut consumes a segment of its input, splice consumes its part.
Independent roots describe disjoint chains in this pool; handles belong to the
indicated chain. Dropping a root does not delete nodes; use clear to reclaim them.

Handles survive allocation and transfers, references do not survive pool relocation.
erase destroys T and recycles its slot: erased handles are invalid even if reused.
Storage follows peak simultaneous live nodes, plus vector capacity; T must support
vector relocation. Do not retain node/payload references across insert/reserve.
Copying the kernel copies the whole pool, not one chain. After moving a kernel,
roots refer to the destination; reinitialize the source before using it again.
No length cache, rank lookup, iterator interface or automatic ownership checks.
*/
template <class T>
struct nlist {
    struct node {
        nidx_t prev = -1, next = -1;
        optional<T> value;
        template <class... A>
        explicit node(in_place_t, A&&... args) : value(in_place, forward<A>(args)...) {}
    };
    struct root {
        nidx_t first = -1, last = -1;
        bool empty() const { return first < 0; }
    };

    narena<node> pool;
    nidx_t free = -1;

    void reserve(nidx_t n) { pool.reserve(n); }
    T& operator[](nidx_t h) { return *pool[h].value; }
    const T& operator[](nidx_t h) const { return *pool[h].value; }
    nidx_t prev(nidx_t h) const { return pool[h].prev; }
    nidx_t next(nidx_t h) const { return pool[h].next; }

    void join(nidx_t left, nidx_t right) {
        if (left >= 0) pool[left].next = right;
        if (right >= 0) pool[right].prev = left;
    }

    /* O(1). [first,last) follows next; last=-1 means through the tail.
       Empty ranges are allowed; otherwise first is live in chain and last is
       reachable. The detached root has no links back into the source. */
    root cut(root& chain, nidx_t first, nidx_t last) {
        if (first == last) return {};
        nidx_t before = prev(first), tail = last < 0 ? chain.last : prev(last);
        join(before, last);
        if (before < 0) chain.first = last;
        if (last < 0) chain.last = before;
        pool[first].prev = pool[tail].next = -1;
        return {first, tail};
    }

    /* O(1), insert part before position (or append at -1), then empty part.
       position belongs to chain; chain and part are disjoint. To move a range
       within one chain, cut first, then splice before a node outside that range. */
    void splice(root& chain, nidx_t position, root& part) {
        if (part.empty()) return;
        nidx_t before = position < 0 ? chain.last : prev(position);
        join(before, part.first);
        join(part.last, position);
        if (before < 0) chain.first = part.first;
        if (position < 0) chain.last = part.last;
        part = {};
    }

    /* Amortized O(1) plus T construction.
       Arguments must not borrow this pool across relocation. */
    template <class... A>
    nidx_t insert(root& chain, nidx_t position, A&&... args) {
        nidx_t h = free;
        if (h < 0) h = pool.make(in_place, forward<A>(args)...);
        else {
            pool[h].value.emplace(forward<A>(args)...);
            free = next(h);
        }
        root part{h, h};
        splice(chain, position, part);
        return h;
    }

    /* O(1) plus T destruction; returns the successor. h is live in chain. */
    nidx_t erase(root& chain, nidx_t h) {
        nidx_t after = next(h);
        cut(chain, h, after);
        pool[h].value.reset();
        pool[h].next = free;
        free = h;
        return after;
    }

    void clear(root& chain) {
        while (!chain.empty()) erase(chain, chain.first);
    }

    /* O(k), changes topology only; payloads and handles are unchanged. */
    void reverse(root& chain) {
        for (nidx_t h = chain.first; h >= 0; h = prev(h))
            swap(pool[h].prev, pool[h].next);
        swap(chain.first, chain.last);
    }
};
