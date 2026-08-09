#pragma once
#include "arena.hpp"
#include "view.hpp"

struct nfhq_noop {
    template <class Q>
    constexpr void operator()(Q&, int) const {}
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
        int left = -1, right = -1, parent = -1, size = 1;
        uint32_t priority;
    };

    narena<node> pool;
    [[no_unique_address]] Pull puller;
    [[no_unique_address]] Push pusher;
    uint64_t random_state;

    explicit nfhq(Pull pull_policy = {}, Push push_policy = {},
                  uint64_t seed = 0x243f6a8885a308d3ULL)
        : puller(move(pull_policy)), pusher(move(push_policy)), random_state(seed) {}

    node& operator[](int handle) { return pool[handle]; }
    const node& operator[](int handle) const { return pool[handle]; }
    int nodes() const { return pool.len(); }
    int size(int root) const { return root < 0 ? 0 : pool[root].size; }
    void reserve(int n) { pool.reserve(n); }

    uint32_t random_priority() {
        uint64_t z = (random_state += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return uint32_t((z ^ (z >> 31)) >> 32);
    }

    template <class U>
    int make(U&& value) {
        int handle = pool.make(node{T(forward<U>(value)), -1, -1, -1, 1,
                                    random_priority()});
        up(handle);
        return handle;
    }

    void down(int handle) {
        if (handle >= 0) invoke(pusher, *this, handle);
    }

    void up(int handle) {
        if (handle < 0) return;
        pool[handle].size = 1 + size(pool[handle].left) + size(pool[handle].right);
        invoke(puller, *this, handle);
    }

    /* Call expose before mutating a saved handle, then rebuild afterwards. */
    void expose(int handle) {
        vector<int> path;
        for (int x = handle; x >= 0; x = pool[x].parent) path.push_back(x);
        for (auto it = path.rbegin(); it != path.rend(); ++it) down(*it);
    }

    void rebuild(int handle) {
        for (; handle >= 0; handle = pool[handle].parent) up(handle);
    }

    void swap_children(int handle) {
        swap(pool[handle].left, pool[handle].right);
    }

  private:
    void set_left(int parent, int child) {
        int old = pool[parent].left;
        if (old >= 0 && old != child && pool[old].parent == parent) pool[old].parent = -1;
        pool[parent].left = child;
        if (child >= 0) pool[child].parent = parent;
    }

    void set_right(int parent, int child) {
        int old = pool[parent].right;
        if (old >= 0 && old != child && pool[old].parent == parent) pool[old].parent = -1;
        pool[parent].right = child;
        if (child >= 0) pool[child].parent = parent;
    }

    int take_left(int parent) {
        int child = pool[parent].left;
        set_left(parent, -1);
        return child;
    }

    int take_right(int parent) {
        int child = pool[parent].right;
        set_right(parent, -1);
        return child;
    }

    template <class F>
    pair<int, int> split_by0(int root, F& goes_left) {
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
    int merge(int left, int right) {
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
            int joined = merge(take_right(left), right);
            set_right(left, joined);
            up(left);
            pool[left].parent = -1;
            return left;
        }
        int joined = merge(left, take_left(right));
        set_left(right, joined);
        up(right);
        pool[right].parent = -1;
        return right;
    }

    pair<int, int> split(int root, int left_size) {
        if (root < 0) return {-1, -1};
        down(root);
        int current_left = size(pool[root].left);
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
    pair<int, int> split_by(int root, F goes_left) {
        return split_by0(root, goes_left);
    }

    int kth(int root, int position) {
        while (true) {
            down(root);
            int left_size = size(pool[root].left);
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

    int root_of(int handle) const {
        while (pool[handle].parent >= 0) handle = pool[handle].parent;
        return handle;
    }

    int rank(int handle) {
        expose(handle);
        int answer = size(pool[handle].left);
        while (pool[handle].parent >= 0) {
            int parent = pool[handle].parent;
            if (pool[parent].right == handle) answer += size(pool[parent].left) + 1;
            handle = parent;
        }
        return answer;
    }

    template <class V>
    int build(V values) {
        int root = -1;
        for (int i = 0; i < values.len(); ++i) root = merge(root, make(values[i]));
        return root;
    }

    /* A sequence descriptor is invalidated semantically when its captured root changes. */
    auto sequence(int root) {
        return ntabulate(size(root), [this, root](int i) -> T& {
            return pool[kth(root, i)].value;
        });
    }
};

template <class T, class Pull = nfhq_noop, class Push = nfhq_noop>
auto nmake_fhq(Pull puller = {}, Push pusher = {},
               uint64_t seed = 0x243f6a8885a308d3ULL) {
    return nfhq<T, Pull, Push>(move(puller), move(pusher), seed);
}
