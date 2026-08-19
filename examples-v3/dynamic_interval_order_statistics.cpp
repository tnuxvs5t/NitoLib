#include "../src-v3/fhq.hpp"
#include "../src-v3/segment.hpp"

/*
The sparse segment topology partitions the value domain.  outer[u].aggregate is the
root of an ordered set of array positions stored in one shared FHQ kernel.  Every
(position,value-segment) membership owns a distinct FHQ node, so active roots attached
to different outer nodes are disjoint.

Positions are zero-based and query intervals are [left,right).  Input is processed
online: no future update value is collected or compressed.  A detached one-node FHQ
root is recycled explicitly; integer handles are stable and no active root shares it.
*/
struct nrange_order_stat {
    using outer_kernel = nsparse_seg<nidx_t, monostate>;
    static constexpr long long value_lo = -1'000'000'000LL;
    static constexpr long long value_hi = 1'000'000'001LL;
    static constexpr nidx_t value_levels =
        bit_width((unsigned long long)(value_hi - value_lo - 1)) + 1;

    nidx_t length, outer_root;
    vector<nidx_t> values;
    outer_kernel outer;
    nfhq<nidx_t> positions;
    vector<nidx_t> free_nodes;

    /* Union of at most paths root-to-leaf paths has this many nodes at most. */
    static nidx_t outer_node_bound(nidx_t paths) {
        long long answer = 0, layer = 1;
        for (nidx_t depth = 0; depth < value_levels; ++depth) {
            answer += min<long long>(layer, paths);
            layer = min<long long>(layer * 2, paths);
        }
        return nidx_t(answer);
    }

    nrange_order_stat(vector<nidx_t> initial, nidx_t operation_bound)
        : length(nidx_t(initial.size())), outer_root(-1), values(move(initial)),
          outer(value_lo, value_hi) {
        nidx_t historical_values = length + operation_bound;
        outer.reserve(outer_node_bound(historical_values));
        positions.reserve(length * value_levels);
        free_nodes.reserve(length);
        outer_root = outer.make(-1);
        for (nidx_t position = 0; position < length; ++position)
            insert_value(values[position], position);
    }

    nidx_t open_child(nidx_t node, nidx_t side) {
        nidx_t next = side ? outer[node].right : outer[node].left;
        if (next < 0) {
            next = outer.make(-1);
            // make may reallocate the arena: re-index node instead of saving a reference.
            if (side) outer[node].right = next;
            else outer[node].left = next;
        }
        return next;
    }

    nidx_t existing_child(nidx_t node, nidx_t side) const {
        return side ? outer[node].right : outer[node].left;
    }

    nidx_t inner_root(nidx_t node) const {
        return node < 0 ? -1 : outer[node].aggregate;
    }

    nidx_t make_position(nidx_t position) {
        if (free_nodes.empty()) return positions.make(position);
        nidx_t handle = free_nodes.back();
        free_nodes.pop_back();
        positions[handle] = {position, -1, -1, -1, 1, positions.random_priority()};
        positions.up(handle);
        return handle;
    }

    nidx_t insert_inner(nidx_t root, nidx_t position) {
        auto [less, greater_equal] = positions.split_by(root,
            [&](nidx_t current) { return current < position; });
        return positions.merge(positions.merge(less, make_position(position)),
                               greater_equal);
    }

    pair<nidx_t, nidx_t> erase_inner(nidx_t root, nidx_t position) {
        auto [less, greater_equal] = positions.split_by(root,
            [&](nidx_t current) { return current < position; });
        auto [equal, greater] = positions.split_by(greater_equal,
            [&](nidx_t current) { return current <= position; });
        auto [erased, remaining_equal] = positions.split(equal, 1);
        nidx_t joined = positions.merge(positions.merge(less, remaining_equal), greater);
        return {joined, erased};
    }

    void insert_value(nidx_t value, nidx_t position) {
        auto child = [&](nidx_t node, nidx_t side) { return open_child(node, side); };
        nsegment_trace(outer_root, value_lo, value_hi, (long long)value, child,
                       [&](nidx_t node) {
            outer[node].aggregate = insert_inner(outer[node].aggregate, position);
        });
    }

    void erase_value(nidx_t value, nidx_t position) {
        auto child = [&](nidx_t node, nidx_t side) { return existing_child(node, side); };
        nsegment_trace(outer_root, value_lo, value_hi, (long long)value, child,
                       [&](nidx_t node) {
            auto [root, erased] = erase_inner(outer[node].aggregate, position);
            outer[node].aggregate = root;
            free_nodes.push_back(erased);
        });
    }

    void assign(nidx_t position, nidx_t value) {
        nidx_t old = values[position];
        if (old == value) return;
        erase_value(old, position);
        insert_value(value, position);
        values[position] = value;
    }

    nidx_t inner_less(nidx_t root, nidx_t position) const {
        nidx_t answer = 0;
        while (root >= 0) {
            const auto& node = positions[root];
            if (node.value < position) {
                answer += positions.size(node.left) + 1;
                root = node.right;
            } else {
                root = node.left;
            }
        }
        return answer;
    }

    nidx_t position_count(nidx_t root, nidx_t query_left, nidx_t query_right) const {
        return inner_less(root, query_right) - inner_less(root, query_left);
    }

    /* Count array values < bound inside the position interval. */
    nidx_t value_count(nidx_t query_left, nidx_t query_right, long long bound) const {
        nidx_t answer = 0;
        auto child = [&](nidx_t node, nidx_t side) { return existing_child(node, side); };
        nsegment_cover(outer_root, value_lo, value_hi, value_lo, bound, child,
                       [&](nidx_t node) {
            answer += position_count(outer[node].aggregate, query_left, query_right);
        });
        return answer;
    }

    nidx_t rank(nidx_t query_left, nidx_t query_right, nidx_t value) const {
        return value_count(query_left, query_right, value) + 1;
    }

    nidx_t kth(nidx_t query_left, nidx_t query_right, nidx_t k) const {
        nidx_t node = outer_root;
        long long left = value_lo, right = value_hi;
        while (left + 1 < right) {
            long long middle = midpoint(left, right);
            nidx_t left_child = outer[node].left;
            nidx_t count_left = position_count(inner_root(left_child), query_left, query_right);
            if (k <= count_left) {
                node = left_child;
                right = middle;
            } else {
                k -= count_left;
                node = outer[node].right;
                left = middle;
            }
        }
        return nidx_t(left);
    }

    nidx_t predecessor(nidx_t query_left, nidx_t query_right, nidx_t value) const {
        nidx_t less = value_count(query_left, query_right, value);
        return kth(query_left, query_right, less);
    }

    nidx_t successor(nidx_t query_left, nidx_t query_right, nidx_t value) const {
        nidx_t less_equal = value_count(query_left, query_right, (long long)value + 1);
        return kth(query_left, query_right, less_equal + 1);
    }
};

#ifndef NITORI_ORDER_STAT_NO_MAIN
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    nidx_t n, q;
    cin >> n >> q;
    vector<nidx_t> initial(n);
    for (nidx_t& value : initial) cin >> value;

    nrange_order_stat tree(move(initial), q);
    while (q--) {
        nidx_t type, left, right, value;
        cin >> type >> left;
        if (type == 3) {
            cin >> value;
            tree.assign(left - 1, value);
            continue;
        }
        cin >> right >> value;
        if (type == 1) cout << tree.rank(left - 1, right, value) << '\n';
        else if (type == 2) cout << tree.kth(left - 1, right, value) << '\n';
        else if (type == 4) cout << tree.predecessor(left - 1, right, value) << '\n';
        else cout << tree.successor(left - 1, right, value) << '\n';
    }
}
#endif
