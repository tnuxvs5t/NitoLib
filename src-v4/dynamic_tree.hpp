#pragma once
#include "fhq.hpp"
#include "hash.hpp"
#include "segment.hpp"

// merge is associative and commutative with id(). link requires different
// components; cut requires a live edge. Removed occurrence handles are not reused.
template <class T, class M = nadd<T>>
struct nett_forest {
    struct item {
        nidx_t vertex;
        bool token;
        T value, aggregate;
        nidx_t vertex_count;
    };

    struct pull_policy {
        [[no_unique_address]] M merge;

        template <class Q>
        void pull(Q& tree, nidx_t handle) {
            auto& node = tree[handle];
            T aggregate = node.left < 0 ? merge.id() : tree[node.left].value.aggregate;
            if (node.value.token)
                aggregate = invoke(merge, move(aggregate), node.value.value);
            if (node.right >= 0)
                aggregate = invoke(merge, move(aggregate),
                                   tree[node.right].value.aggregate);
            node.value.aggregate = move(aggregate);
            node.value.vertex_count = (node.value.token ? 1 : 0) +
                (node.left < 0 ? 0 : tree[node.left].value.vertex_count) +
                (node.right < 0 ? 0 : tree[node.right].value.vertex_count);
        }
    };

    using kernel_type = nfhq<item, pull_policy>;
    using edge_key = pair<nidx_t, nidx_t>;

    kernel_type sequence;
    vector<nidx_t> representative;
    unordered_map<edge_key, pair<nidx_t, nidx_t>, nhash> occurrence;

    explicit nett_forest(nidx_t n = 0, M merge = {})
        : sequence(pull_policy{move(merge)}), representative(n) {
        T identity = sequence.ops.merge.id();
        for (nidx_t vertex = 0; vertex < n; ++vertex)
            representative[vertex] = sequence.make(
                item{vertex, true, identity, identity, 1});
    }

    template <class V>
    requires requires(V& source) { source[0]; }
    explicit nett_forest(const V& values, M merge = {})
        : sequence(pull_policy{move(merge)}), representative(nlen(values)) {
        T identity = sequence.ops.merge.id();
        for (nidx_t vertex = 0; vertex < nlen(values); ++vertex)
            representative[vertex] = sequence.make(
                item{vertex, true, values[vertex], identity, 1});
    }

    nidx_t len() const { return nidx_t(representative.size()); }

    static edge_key key(nidx_t a, nidx_t b) {
        if (a > b) swap(a, b);
        return {a, b};
    }

    nidx_t root(nidx_t vertex) const {
        return sequence.root_of(representative[vertex]);
    }
    bool connected(nidx_t a, nidx_t b) const { return root(a) == root(b); }
    nidx_t component_size(nidx_t vertex) const {
        return sequence[root(vertex)].value.vertex_count;
    }
    T fold(nidx_t vertex) const { return sequence[root(vertex)].value.aggregate; }

    void set(nidx_t vertex, T value) {
        nidx_t handle = representative[vertex];
        sequence.expose(handle);
        sequence[handle].value.value = move(value);
        sequence.rebuild(handle);
    }

    nidx_t reroot(nidx_t vertex) {
        nidx_t handle = representative[vertex];
        nidx_t tree = sequence.root_of(handle);
        nidx_t position = sequence.rank(handle);
        auto [left, right] = sequence.split(tree, position);
        return sequence.merge(right, left);
    }

    void link(nidx_t a, nidx_t b) {
        nidx_t left = reroot(a), right = reroot(b);
        T identity = sequence.ops.merge.id();
        nidx_t ab = sequence.make(item{-1, false, identity, identity, 0});
        nidx_t ba = sequence.make(item{-1, false, identity, identity, 0});
        sequence.merge(sequence.merge(sequence.merge(left, ab), right), ba);
        occurrence[key(a, b)] = {ab, ba};
    }

    void cut(nidx_t a, nidx_t b) {
        auto [first, second] = occurrence.at(key(a, b));
        nidx_t tree = sequence.root_of(first);
        nidx_t left = sequence.rank(first), right = sequence.rank(second);
        if (left > right) swap(left, right);
        auto [through_right, suffix] = sequence.split(tree, right + 1);
        auto [prefix, right_edge] = sequence.split(through_right, right);
        auto [through_left, middle] = sequence.split(prefix, left + 1);
        auto [outside_prefix, left_edge] = sequence.split(through_left, left);
        sequence.merge(suffix, outside_prefix);
        (void)middle;
        (void)right_edge;
        (void)left_edge;
        occurrence.erase(key(a, b));
    }
};
