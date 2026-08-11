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
    using outer_kernel = nsparse_seg<int, monostate>;
    static constexpr long long value_lo = -1'000'000'000LL;
    static constexpr long long value_hi = 1'000'000'001LL;
    static constexpr int value_levels =
        bit_width((unsigned long long)(value_hi - value_lo - 1)) + 1;

    int length, outer_root;
    vector<int> values;
    outer_kernel outer;
    nfhq<int> positions;
    vector<int> free_nodes;

    /* Union of at most paths root-to-leaf paths has this many nodes at most. */
    static int outer_node_bound(int paths) {
        long long answer = 0, layer = 1;
        for (int depth = 0; depth < value_levels; ++depth) {
            answer += min<long long>(layer, paths);
            layer = min<long long>(layer * 2, paths);
        }
        return int(answer);
    }

    nrange_order_stat(vector<int> initial, int operation_bound)
        : length(int(initial.size())), outer_root(-1), values(move(initial)),
          outer(value_lo, value_hi) {
        int historical_values = length + operation_bound;
        outer.reserve(outer_node_bound(historical_values));
        positions.reserve(length * value_levels);
        free_nodes.reserve(length);
        outer_root = outer.make(-1);
        for (int position = 0; position < length; ++position)
            insert_value(values[position], position);
    }

    int open_child(int node, int side) {
        int next = side ? outer[node].right : outer[node].left;
        if (next < 0) {
            next = outer.make(-1);
            // make may reallocate the arena: re-index node instead of saving a reference.
            if (side) outer[node].right = next;
            else outer[node].left = next;
        }
        return next;
    }

    int existing_child(int node, int side) const {
        return side ? outer[node].right : outer[node].left;
    }

    int inner_root(int node) const {
        return node < 0 ? -1 : outer[node].aggregate;
    }

    int make_position(int position) {
        if (free_nodes.empty()) return positions.make(position);
        int handle = free_nodes.back();
        free_nodes.pop_back();
        positions[handle] = {position, -1, -1, -1, 1, positions.random_priority()};
        positions.up(handle);
        return handle;
    }

    int insert_inner(int root, int position) {
        auto [less, greater_equal] = positions.split_by(root,
            [&](int current) { return current < position; });
        return positions.merge(positions.merge(less, make_position(position)),
                               greater_equal);
    }

    pair<int, int> erase_inner(int root, int position) {
        auto [less, greater_equal] = positions.split_by(root,
            [&](int current) { return current < position; });
        auto [equal, greater] = positions.split_by(greater_equal,
            [&](int current) { return current <= position; });
        auto [erased, remaining_equal] = positions.split(equal, 1);
        int joined = positions.merge(positions.merge(less, remaining_equal), greater);
        return {joined, erased};
    }

    void insert_value(int value, int position) {
        auto child = [&](int node, int side) { return open_child(node, side); };
        nsegment_trace(outer_root, value_lo, value_hi, (long long)value, child,
                       [&](int node) {
            outer[node].aggregate = insert_inner(outer[node].aggregate, position);
        });
    }

    void erase_value(int value, int position) {
        auto child = [&](int node, int side) { return existing_child(node, side); };
        nsegment_trace(outer_root, value_lo, value_hi, (long long)value, child,
                       [&](int node) {
            auto [root, erased] = erase_inner(outer[node].aggregate, position);
            outer[node].aggregate = root;
            free_nodes.push_back(erased);
        });
    }

    void assign(int position, int value) {
        int old = values[position];
        if (old == value) return;
        erase_value(old, position);
        insert_value(value, position);
        values[position] = value;
    }

    int inner_less(int root, int position) const {
        int answer = 0;
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

    int position_count(int root, int query_left, int query_right) const {
        return inner_less(root, query_right) - inner_less(root, query_left);
    }

    /* Count array values < bound inside the position interval. */
    int value_count(int query_left, int query_right, long long bound) const {
        int answer = 0;
        auto child = [&](int node, int side) { return existing_child(node, side); };
        nsegment_cover(outer_root, value_lo, value_hi, value_lo, bound, child,
                       [&](int node) {
            answer += position_count(outer[node].aggregate, query_left, query_right);
        });
        return answer;
    }

    int rank(int query_left, int query_right, int value) const {
        return value_count(query_left, query_right, value) + 1;
    }

    int kth(int query_left, int query_right, int k) const {
        int node = outer_root;
        long long left = value_lo, right = value_hi;
        while (left + 1 < right) {
            long long middle = midpoint(left, right);
            int left_child = outer[node].left;
            int count_left = position_count(inner_root(left_child), query_left, query_right);
            if (k <= count_left) {
                node = left_child;
                right = middle;
            } else {
                k -= count_left;
                node = outer[node].right;
                left = middle;
            }
        }
        return int(left);
    }

    int predecessor(int query_left, int query_right, int value) const {
        int less = value_count(query_left, query_right, value);
        return kth(query_left, query_right, less);
    }

    int successor(int query_left, int query_right, int value) const {
        int less_equal = value_count(query_left, query_right, (long long)value + 1);
        return kth(query_left, query_right, less_equal + 1);
    }
};

#ifndef NITORI_ORDER_STAT_NO_MAIN
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, q;
    cin >> n >> q;
    vector<int> initial(n);
    for (int& value : initial) cin >> value;

    nrange_order_stat tree(move(initial), q);
    while (q--) {
        int type, left, right, value;
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
