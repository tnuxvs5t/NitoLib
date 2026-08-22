#pragma once
#include "fhq.hpp"

/*
nbag is an ordered multiset over one nfhq<T>.  C must be a strict weak ordering;
equivalent values are kept as separate nodes and inserted after existing equivalents.
The returned insert handle belongs to this bag until that node is erased or clear runs;
a detached handle is not a value iterator.  Positions are zero-based and all bounds are
[left,right).

T is always explicit: source expressions may yield proxy or nested-reference values.
Value queries descend without reshaping the tree, so a sequence remains valid across
read queries; insert/emplace, erase operations and clear invalidate it.  Insert, erase
and value queries are expected O(log n), including each kth/sequence access.  The arena
is append-only between clear calls, so erased nodes consume storage until clear().
Handles survive arena relocation; element references do not.
*/
template <class T, class C = less<>>
struct nbag {
    static constexpr uint64_t default_seed = 0x243f6a8885a308d3ULL;

    struct pieces {
        nidx_t left, equal, right;
    };

    nfhq<T> tree;
    [[no_unique_address]] mutable C compare;
    nidx_t root = -1;

    explicit nbag(C order = {}, uint64_t seed = default_seed)
        : tree(nfhq_noop{}, nfhq_noop{}, seed), compare(move(order)) {}

    template <class V>
    requires requires(V& source) {
        source.len();
        source[0];
    }
    explicit nbag(V source, C order = {}, uint64_t seed = default_seed)
        : nbag(move(order), seed) {
        for (nidx_t i = 0; i < source.len(); ++i) insert(T(source[i]));
    }

    nidx_t len() const { return tree.size(root); }
    bool empty() const { return root < 0; }
    nidx_t nodes() const { return tree.nodes(); }
    void reserve(nidx_t count) { tree.reserve(count); }

private:
    nidx_t cut(const T& key, bool upper) const {
        nidx_t answer = 0;
        for (nidx_t handle = root; handle >= 0;) {
            const auto& node = tree[handle];
            if (invoke(compare, node.value, key) ||
                (upper && !invoke(compare, key, node.value))) {
                answer += tree.size(node.left) + 1;
                handle = node.right;
            } else {
                handle = node.left;
            }
        }
        return answer;
    }

    nidx_t kth_handle(nidx_t handle, nidx_t position) const {
        while (true) {
            const auto& node = tree[handle];
            nidx_t left_size = tree.size(node.left);
            if (position < left_size)
                handle = node.left;
            else if (position == left_size)
                return handle;
            else
                position -= left_size + 1, handle = node.right;
        }
    }

    pieces split_equal(const T& key) {
        auto [left, not_less] = tree.split_by(root, [this, &key](const T& value) {
            return invoke(compare, value, key);
        });
        auto [equal, right] = tree.split_by(not_less, [this, &key](const T& value) {
            return !invoke(compare, key, value);
        });
        return {left, equal, right};
    }

    void restore(pieces part) {
        root = tree.merge(part.left, tree.merge(part.equal, part.right));
    }

public:
    /* Insert after equivalent values and return the nfhq handle. */
    nidx_t insert(T value) {
        auto [left, right] = tree.split_by(root, [this, &value](const T& current) {
            return !invoke(compare, value, current);
        });
        nidx_t handle = tree.make(move(value));
        root = tree.merge(tree.merge(left, handle), right);
        return handle;
    }

    template <class... A>
    nidx_t emplace(A&&... args) {
        return insert(T(forward<A>(args)...));
    }

    nidx_t lower_bound(const T& key) const { return cut(key, false); }
    nidx_t upper_bound(const T& key) const { return cut(key, true); }
    nidx_t order_of_key(const T& key) const { return lower_bound(key); }

    pair<nidx_t, nidx_t> equal_range(const T& key) const {
        return {lower_bound(key), upper_bound(key)};
    }

    nidx_t count(const T& key) const {
        auto [left, right] = equal_range(key);
        return right - left;
    }

    nidx_t find(const T& key) const {
        nidx_t position = lower_bound(key);
        return position < len() && !invoke(compare, key, (*this)[position]) ? position : len();
    }

    bool contains(const T& key) const { return find(key) != len(); }

    bool erase_one(const T& key) {
        pieces part = split_equal(key);
        if (part.equal < 0) {
            restore(part);
            return false;
        }
        auto [removed, rest] = tree.split(part.equal, 1);
        (void)removed;
        root = tree.merge(part.left, tree.merge(rest, part.right));
        return true;
    }

    nidx_t erase_all(const T& key) {
        pieces part = split_equal(key);
        nidx_t removed = tree.size(part.equal);
        root = tree.merge(part.left, part.right);
        return removed;
    }

    /* position must satisfy 0 <= position < len(); the removed value is moved out. */
    T erase_at(nidx_t position) {
        auto [left, tail] = tree.split(root, position);
        auto [one, right] = tree.split(tail, 1);
        nidx_t handle = tree.kth(one, 0);
        T value = move(tree[handle].value);
        root = tree.merge(left, right);
        return value;
    }

    /* handle must currently belong to this bag; false also covers detached handles. */
    bool erase_handle(nidx_t handle) {
        if (handle < 0 || handle >= tree.nodes() || root < 0 || tree.root_of(handle) != root)
            return false;
        erase_at(tree.rank(handle));
        return true;
    }

    void clear() {
        root = -1;
        tree.pool.data.clear();
    }

    const T& kth(nidx_t position) const {
        return tree[kth_handle(root, position)].value;
    }

    const T& operator[](nidx_t position) const { return kth(position); }
    const T& front() const { return kth(0); }
    const T& back() const { return kth(len() - 1); }

    /* Read-only positional view; structural mutation invalidates it, read queries do not. */
    auto sequence() const {
        nidx_t captured_root = root;
        return ntabulate(tree.size(captured_root), [this, captured_root](nidx_t i) -> const T& {
            return tree[kth_handle(captured_root, i)].value;
        });
    }
};
