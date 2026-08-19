#pragma once
#include "arena.hpp"
#include "view.hpp"

struct nfhq_noop {
    template <class Q>
    constexpr void operator()(Q&, nidx_t) const {}
};

/*
One nfhq is one node arena and one policy state; any number of disjoint roots may live
inside it.  -1 is the null root.  split/merge are destructive root algebra: input roots
must belong to this kernel, merge inputs must be disjoint, and consumed root variables
must not be used as independent trees afterwards.  These contracts are intentionally
not encoded through owners, domains, epochs or concepts.

Pull and push, when present, are called as policy(*this, handle).  pull observes an
already-correct structural size.  push may update payloads and may call swap_children;
it must leave the same node set and valid lazy representation.  Neither callback may
retain node references across allocation.
*/
template <class T, class Pull = nfhq_noop, class Push = nfhq_noop>
struct nfhq {
    struct node {
        T value;
        nidx_t left = -1, right = -1, parent = -1, size = 1;
        uint32_t priority;
    };

    narena<node> pool;
    [[no_unique_address]] Pull puller;
    [[no_unique_address]] Push pusher;
    uint64_t random_state;

    explicit nfhq(Pull pull_policy = {}, Push push_policy = {},
                  uint64_t seed = 0x243f6a8885a308d3ULL)
        : puller(move(pull_policy)), pusher(move(push_policy)), random_state(seed) {}

    node& operator[](nidx_t handle) { return pool[handle]; }
    const node& operator[](nidx_t handle) const { return pool[handle]; }
    nidx_t nodes() const { return pool.len(); }
    nidx_t size(nidx_t root) const { return root < 0 ? 0 : pool[root].size; }
    void reserve(nidx_t n) { pool.reserve(n); }

    uint32_t random_priority() {
        uint64_t z = (random_state += 0x9e3779b97f4a7c15ULL);
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
        if (handle >= 0) invoke(pusher, *this, handle);
    }

    void up(nidx_t handle) {
        if (handle < 0) return;
        pool[handle].size = 1 + size(pool[handle].left) + size(pool[handle].right);
        invoke(puller, *this, handle);
    }

    /* Call expose before mutating a saved handle, then rebuild afterwards. */
    void expose(nidx_t handle) {
        vector<nidx_t> path;
        for (nidx_t x = handle; x >= 0; x = pool[x].parent) path.push_back(x);
        for (auto it = path.rbegin(); it != path.rend(); ++it) down(*it);
    }

    void rebuild(nidx_t handle) {
        for (; handle >= 0; handle = pool[handle].parent) up(handle);
    }

    void swap_children(nidx_t handle) {
        swap(pool[handle].left, pool[handle].right);
    }

  private:
    void set_left(nidx_t parent, nidx_t child) {
        nidx_t old = pool[parent].left;
        if (old >= 0 && old != child && pool[old].parent == parent) pool[old].parent = -1;
        pool[parent].left = child;
        if (child >= 0) pool[child].parent = parent;
    }

    void set_right(nidx_t parent, nidx_t child) {
        nidx_t old = pool[parent].right;
        if (old >= 0 && old != child && pool[old].parent == parent) pool[old].parent = -1;
        pool[parent].right = child;
        if (child >= 0) pool[child].parent = parent;
    }

    nidx_t take_left(nidx_t parent) {
        nidx_t child = pool[parent].left;
        set_left(parent, -1);
        return child;
    }

    nidx_t take_right(nidx_t parent) {
        nidx_t child = pool[parent].right;
        set_right(parent, -1);
        return child;
    }

    template <class F>
    pair<nidx_t, nidx_t> split_by0(nidx_t root, F& goes_left) {
        if (root < 0) return {-1, -1};
        down(root);
        if (invoke(goes_left, pool[root].value)) {
            auto [middle, right] = split_by0(take_right(root), goes_left);
            set_right(root, middle);
            up(root);
            pool[root].parent = -1;
            return {root, right};
        }
        auto [left, middle] = split_by0(take_left(root), goes_left);
        set_left(root, middle);
        up(root);
        pool[root].parent = -1;
        return {left, root};
    }

  public:
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
            nidx_t joined = merge(take_right(left), right);
            set_right(left, joined);
            up(left);
            pool[left].parent = -1;
            return left;
        }
        nidx_t joined = merge(left, take_left(right));
        set_left(right, joined);
        up(right);
        pool[right].parent = -1;
        return right;
    }

    pair<nidx_t, nidx_t> split(nidx_t root, nidx_t left_size) {
        if (root < 0) return {-1, -1};
        down(root);
        nidx_t current_left = size(pool[root].left);
        if (left_size <= current_left) {
            auto [left, middle] = split(take_left(root), left_size);
            set_left(root, middle);
            up(root);
            pool[root].parent = -1;
            return {left, root};
        }
        auto [middle, right] = split(take_right(root), left_size - current_left - 1);
        set_right(root, middle);
        up(root);
        pool[root].parent = -1;
        return {root, right};
    }

    /* goes_left(value) must be false only after it first becomes false in inorder. */
    template <class F>
    pair<nidx_t, nidx_t> split_by(nidx_t root, F goes_left) {
        return split_by0(root, goes_left);
    }

    nidx_t kth(nidx_t root, nidx_t position) {
        while (true) {
            down(root);
            nidx_t left_size = size(pool[root].left);
            if (position < left_size)
                root = pool[root].left;
            else if (position == left_size)
                return root;
            else {
                position -= left_size + 1;
                root = pool[root].right;
            }
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
    nidx_t build(V values) {
        nidx_t root = -1;
        for (nidx_t i = 0; i < values.len(); ++i) root = merge(root, make(values[i]));
        return root;
    }

    /* A sequence descriptor is invalidated semantically when its captured root changes. */
    auto sequence(nidx_t root) {
        return ntabulate(size(root), [this, root](nidx_t i) -> T& {
            return pool[kth(root, i)].value;
        });
    }
};

template <class T, class Pull = nfhq_noop, class Push = nfhq_noop>
auto nmake_fhq(Pull puller = {}, Push pusher = {},
               uint64_t seed = 0x243f6a8885a308d3ULL) {
    return nfhq<T, Pull, Push>(move(puller), move(pusher), seed);
}
