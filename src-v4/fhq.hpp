#pragma once
#include "arena.hpp"
#include "view.hpp"

struct nfhq_noop {};

/*
   One kernel owns one arena and one policy.  Roots are destructive values: split
   consumes its input root into two roots, merge consumes two disjoint roots, and
   -1 is the null root.  Policy callbacks see handles, never node references kept
   across allocation.
*/
template <class T, class Ops = nfhq_noop>
struct nfhq {
    struct node {
        T value;
        nidx_t left = -1, right = -1, parent = -1, size = 1;
        uint32_t priority;
    };

    narena<node> pool;
    [[no_unique_address]] Ops ops;
    uint64_t random_state;

    explicit nfhq(Ops policy = {}, uint64_t seed = 0x243f6a8885a308d3ULL)
        : ops(move(policy)), random_state(seed) {}

    node& operator[](nidx_t handle) { return pool[handle]; }
    const node& operator[](nidx_t handle) const { return pool[handle]; }
    nidx_t nodes() const { return pool.len(); }
    nidx_t size(nidx_t root) const { return root < 0 ? 0 : pool[root].size; }
    void reserve(nidx_t n) { pool.reserve(n); }

    uint32_t random_priority() {
        uint64_t z = random_state += 0x9e3779b97f4a7c15ULL;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return uint32_t((z ^ (z >> 31)) >> 32);
    }

    template <class U>
    nidx_t make(U&& value) {
        nidx_t handle = pool.make(node{T(forward<U>(value)), -1, -1, -1, 1,
                                       random_priority()});
        up(handle);
        return handle;
    }

    void down(nidx_t handle) {
        if constexpr (requires { ops.push(*this, handle); })
            if (handle >= 0) ops.push(*this, handle);
    }

    void up(nidx_t handle) {
        if (handle < 0) return;
        pool[handle].size = 1 + size(pool[handle].left) + size(pool[handle].right);
        if constexpr (requires { ops.pull(*this, handle); }) ops.pull(*this, handle);
    }

    void expose(nidx_t handle) {
        if (handle < 0) return;
        expose(pool[handle].parent);
        down(handle);
    }

    void rebuild(nidx_t handle) {
        for (; handle >= 0; handle = pool[handle].parent) up(handle);
    }

    void swap_children(nidx_t handle) {
        swap(pool[handle].left, pool[handle].right);
    }

private:
    void set_child(nidx_t parent, nidx_t child, bool right) {
        nidx_t& link = right ? pool[parent].right : pool[parent].left;
        nidx_t old = link;
        if (old >= 0 && old != child && pool[old].parent == parent)
            pool[old].parent = -1;
        link = child;
        if (child >= 0) pool[child].parent = parent;
    }

    nidx_t take_child(nidx_t parent, bool right) {
        nidx_t child = right ? pool[parent].right : pool[parent].left;
        set_child(parent, -1, right);
        return child;
    }

    template <class F>
    pair<nidx_t, nidx_t> split0(nidx_t root, F& goes_left) {
        if (root < 0) return {-1, -1};
        down(root);
        if (invoke(goes_left, root)) {
            auto [middle, right] = split0(take_child(root, true), goes_left);
            set_child(root, middle, true);
            up(root);
            pool[root].parent = -1;
            return {root, right};
        }
        auto [left, middle] = split0(take_child(root, false), goes_left);
        set_child(root, middle, false);
        up(root);
        pool[root].parent = -1;
        return {left, root};
    }

public:
    template <class V, class E>
    void walk(nidx_t root, V&& visit, E&& element) {
        if (root < 0 || invoke(visit, *this, root)) return;
        down(root);
        walk(pool[root].left, visit, element);
        invoke(element, *this, root);
        walk(pool[root].right, visit, element);
        up(root);
    }

    template <class C>
    void apply(nidx_t root, const C& command) {
        walk(root,
             [&](auto&, nidx_t handle) {
                 bool done = ops.try_apply(*this, handle, command);
                 assert(done || size(handle) > 1);
                 return done;
             },
             [&](auto&, nidx_t handle) { ops.apply_one(*this, handle, command); });
    }

    template <class E>
    nidx_t edit(nidx_t root, nidx_t left, nidx_t right, E&& change) {
        auto [prefix, suffix] = split(root, right);
        auto [head, middle] = split(prefix, left);
        nidx_t replacement = invoke(change, *this, middle);
        return merge(merge(head, replacement), suffix);
    }

    nidx_t merge(nidx_t left, nidx_t right) {
        if (left < 0) {
            if (right >= 0) pool[right].parent = -1;
            return right;
        }
        if (right < 0) {
            pool[left].parent = -1;
            return left;
        }
        down(left);
        down(right);
        if (pool[left].priority >= pool[right].priority) {
            nidx_t joined = merge(take_child(left, true), right);
            set_child(left, joined, true);
            up(left);
            pool[left].parent = -1;
            return left;
        }
        nidx_t joined = merge(left, take_child(right, false));
        set_child(right, joined, false);
        up(right);
        pool[right].parent = -1;
        return right;
    }

    pair<nidx_t, nidx_t> split(nidx_t root, nidx_t left_size) {
        auto before = [&](nidx_t handle) {
            nidx_t width = size(pool[handle].left) + 1;
            if (left_size < width) return false;
            left_size -= width;
            return true;
        };
        return split0(root, before);
    }

    template <class F>
    pair<nidx_t, nidx_t> split_by(nidx_t root, F goes_left) {
        auto before = [&](nidx_t handle) {
            return invoke(goes_left, pool[handle].value);
        };
        return split0(root, before);
    }

    nidx_t kth(nidx_t root, nidx_t position) {
        while (true) {
            down(root);
            nidx_t left_size = size(pool[root].left);
            if (position < left_size) root = pool[root].left;
            else if (position == left_size) return root;
            else position -= left_size + 1, root = pool[root].right;
        }
    }

    nidx_t root_of(nidx_t handle) const {
        while (pool[handle].parent >= 0) handle = pool[handle].parent;
        return handle;
    }

    nidx_t rank(nidx_t handle) {
        expose(handle);
        nidx_t answer = size(pool[handle].left);
        while (pool[handle].parent >= 0) {
            nidx_t parent = pool[handle].parent;
            if (pool[parent].right == handle) answer += size(pool[parent].left) + 1;
            handle = parent;
        }
        return answer;
    }

    template <class V>
    nidx_t build(V&& values) {
        nidx_t root = -1;
        for (nidx_t i = 0; i < nlen(values); ++i) root = merge(root, make(values[i]));
        return root;
    }

    auto sequence(nidx_t root) {
        return ntabulate(size(root), [this, root](nidx_t i) -> T& {
            return pool[kth(root, i)].value;
        });
    }
};
