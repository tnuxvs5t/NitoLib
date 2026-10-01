#pragma once
#include "arena.hpp"

struct nreftree_noop {
    monostate join(const auto&, nidx_t, nidx_t, nidx_t) const { return {}; }
};

/*
Immutable binary patterns. A finite object is (root,H), covering [0,2^H), with
0 <= node.height <= H <= 60. Missing upper levels REPEAT the pattern, not zero-pad.
Leaves have height zero; every leaf() creates a distinct terminal, even for equal
values. There is no empty root or distinguished Boolean handle. All input handles
belong to this kernel. Children have strictly smaller stored heights than parents.

Info is owned information at the node's STORED height. ops.join(const q&,H,l,r)
constructs Info for a new branch, interpreting both children at H-1. No equality,
hash, identity or lift interface is required. The policy must respect repetition:
join(H,p,p) returns p without calling it. Policies may keep caches/counters but must
not change existing nodes, recursively allocate into q, or change the meaning of
published information. Position/context-dependent information belongs outside q.

Only const node access is exposed. Info must not borrow relocatable pool storage;
use handles. Allocation/reserve can invalidate references, never handles. Copying
the kernel (when Info/Ops permit it) copies the entire pool; copying a root is O(1).
Storage retains every allocated node. There is no interning or reclamation, and
unequal handles need not denote unequal content. Queries allocate no tree nodes.
*/
template <class Info = monostate, class Ops = nreftree_noop>
class nreftree {
  public:
    using U = uint64_t; // Logical addresses are independent of physical handle width.
    struct node {
        [[no_unique_address]] Info value;
        nidx_t left, right, height;
    };

  private:
    narena<node> pool;

  public:
    [[no_unique_address]] Ops ops;

    explicit nreftree(Ops policy = {}) : ops(move(policy)) {}
    const node& operator[](nidx_t p) const { return pool[p]; }
    nidx_t nodes() const { return pool.len(); }
    void reserve(nidx_t n) { pool.reserve(n); }

    template <class... A>
    nidx_t leaf(A&&... args) {
        return pool.make(node{Info(forward<A>(args)...), -1, -1, 0});
    }

    /* Logical halves, each observed at H-1. Requires 0 < H <= 60. O(1). */
    pair<nidx_t, nidx_t> split(nidx_t p, nidx_t H) const {
        return pool[p].height == H ? pair{pool[p].left, pool[p].right} : pair{p, p};
    }

    /* Children have height < H <= 60. Amortized O(1) plus policy/Info costs.
       Reduction compares handles only; it preserves ordered, noncommutative data. */
    nidx_t join(nidx_t H, nidx_t left, nidx_t right) {
        if (left == right) return left;
        Info value = ops.join(as_const(*this), H, left, right);
        return pool.make(node{move(value), left, right, H});
    }

    /* Valid address x in the caller's observation domain. O(stored path height). */
    nidx_t leaf_at(nidx_t p, U x) const {
        while (pool[p].height) {
            const auto& a = pool[p];
            p = ((x >> (a.height - 1)) & 1) ? a.right : a.left;
        }
        return p;
    }

    /* x is 2^k-aligned and [x,x+2^k) lies in a valid observation of p.
       0 <= k <= 60. Returns a pattern observed at k, allocating nothing. */
    nidx_t block(nidx_t p, U x, nidx_t k) const {
        while (pool[p].height > k) {
            const auto& a = pool[p];
            p = ((x >> (a.height - 1)) & 1) ? a.right : a.left;
        }
        return p;
    }

    /* Persistent aligned replacement. 0 <= k <= H <= 60, height(p) <= H,
       height(src) <= k, x < 2^H and x divisible by 2^k. src repeats up to k.
       All old roots survive. At most H-k new nodes; O(H-k+1) structural work
       amortized, plus policy/Info costs. Recursion uses O(H-k+1) stack. */
    nidx_t paste(nidx_t p, nidx_t H, U x, nidx_t k, nidx_t src) {
        if (H == k || p == src) return src;
        auto [left, right] = split(p, H);
        U half = U(1) << (H - 1);
        if (x < half) left = paste(left, H - 1, x, k, src);
        else right = paste(right, H - 1, x - half, k, src);
        return join(H, left, right);
    }

    /* terminal is a height-zero root, not an Info value. */
    nidx_t set(nidx_t p, nidx_t H, U x, nidx_t terminal) {
        return paste(p, H, x, 0, terminal);
    }

    /* First qualifying address >= x, or 2^H if none (also for x >= 2^H).
       has(p,h) answers EXACTLY whether the pattern observed at h contains a
       qualifying position. Its meaning respects logical splitting/repetition;
       it cannot depend on a hidden absolute occurrence offset. O(H+1) predicate
       calls/structural work, O(H+1) stack. The callback must not mutate q.
       Approximate pruning needs a separate traversal via split(), not this port. */
    template <class P>
    U find_first(nidx_t p, nidx_t H, U x, P&& has) const {
        auto search = [&](auto&& self, nidx_t root, nidx_t h, U start) -> U {
            U end = U(1) << h;
            if (start >= end || !invoke(has, root, h)) return end;
            if (!h) return 0;
            auto [left, right] = split(root, h);
            U half = end / 2;
            if (start < half) {
                U answer = self(self, left, h - 1, start);
                if (answer < half) return answer;
                start = 0;
            } else start -= half;
            return half + self(self, right, h - 1, start);
        };
        return search(search, p, H, x);
    }

    /* Zero-based selection. count(p,h) is the exact number of qualifying logical
       positions (0/1 at a leaf), additive across split(). Requires k<count(p,H).
       Callback must not mutate q. O(H) calls/work, O(1) extra space. */
    template <class C>
    U kth(nidx_t p, nidx_t H, U k, C&& count) const {
        U offset = 0;
        while (H) {
            auto [left, right] = split(p, H);
            U amount = invoke(count, left, H - 1);
            --H;
            if (k < amount) p = left;
            else { k -= amount; p = right; offset += U(1) << H; }
        }
        return offset;
    }
};
