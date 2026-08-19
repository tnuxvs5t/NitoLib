#define NITORI_ORDER_STAT_NO_MAIN
#include "../examples-v3/dynamic_interval_order_statistics.cpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

static vector<nidx_t> sorted_slice(const vector<nidx_t>& values, nidx_t left, nidx_t right) {
    vector<nidx_t> result(values.begin() + left, values.begin() + right);
    sort(result.begin(), result.end());
    return result;
}

/* Verify active FHQ roots, cross-root disjointness, and the value-segment invariant. */
static void verify_structure(const nrange_order_stat& tree, const vector<nidx_t>& reference,
                             nidx_t historical_values) {
    vector<unsigned char> seen(tree.positions.nodes());
    auto read_inner = [&](auto&& self, nidx_t root, nidx_t parent, vector<nidx_t>& values) -> nidx_t {
        if (root < 0) return 0;
        CHECK(!seen[root]);
        seen[root] = true;
        const auto& node = tree.positions[root];
        CHECK(node.parent == parent);
        if (node.left >= 0) CHECK(node.priority >= tree.positions[node.left].priority);
        if (node.right >= 0) CHECK(node.priority >= tree.positions[node.right].priority);
        nidx_t left_size = self(self, node.left, root, values);
        values.push_back(node.value);
        nidx_t right_size = self(self, node.right, root, values);
        CHECK(node.size == left_size + right_size + 1);
        return node.size;
    };

    auto check_outer = [&](auto&& self, nidx_t node, long long left, long long right) -> void {
        CHECK(node >= 0);
        vector<nidx_t> expected;
        for (nidx_t position = 0; position < nidx_t(reference.size()); ++position)
            if (left <= reference[position] && reference[position] < right)
                expected.push_back(position);
        vector<nidx_t> actual;
        read_inner(read_inner, tree.outer[node].aggregate, -1, actual);
        CHECK(actual == expected);

        if (left + 1 == right) {
            CHECK(tree.outer[node].left < 0 && tree.outer[node].right < 0);
            return;
        }
        long long middle = midpoint(left, right);
        bool need_left = any_of(reference.begin(), reference.end(),
            [&](nidx_t value) { return left <= value && value < middle; });
        bool need_right = any_of(reference.begin(), reference.end(),
            [&](nidx_t value) { return middle <= value && value < right; });
        nidx_t left_child = tree.outer[node].left, right_child = tree.outer[node].right;
        CHECK(!need_left || left_child >= 0);
        CHECK(!need_right || right_child >= 0);
        if (left_child >= 0) self(self, left_child, left, middle);
        if (right_child >= 0) self(self, right_child, middle, right);
    };
    check_outer(check_outer, tree.outer_root,
                nrange_order_stat::value_lo, nrange_order_stat::value_hi);
    for (nidx_t handle : tree.free_nodes) {
        CHECK(!seen[handle]);
        seen[handle] = true;
        const auto& node = tree.positions[handle];
        CHECK(node.left < 0 && node.right < 0 && node.parent < 0 && node.size == 1);
    }
    CHECK(count(seen.begin(), seen.end(), 1) == tree.positions.nodes());
    CHECK(tree.outer.nodes() <= nrange_order_stat::outer_node_bound(historical_values));
}

int main() {
    {
        vector<nidx_t> reference{-1'000'000'000, 0, 0, 1'000'000'000};
        nrange_order_stat tree(reference, 5);
        CHECK(tree.rank(0, 4, 0) == 2);
        CHECK(tree.rank(0, 4, -1'000'000'000) == 1);
        CHECK(tree.kth(0, 4, 3) == 0);
        CHECK(tree.predecessor(0, 4, 0) == -1'000'000'000);
        CHECK(tree.successor(0, 4, 0) == 1'000'000'000);
        tree.assign(0, 1'000'000'000);
        reference[0] = 1'000'000'000;
        CHECK(tree.predecessor(0, 4, 1'000'000'000) == 0);
        CHECK(tree.successor(0, 4, -1'000'000'000) == 0);
        nidx_t outer_before = tree.outer.nodes(), inner_before = tree.positions.nodes();
        tree.assign(0, 1'000'000'000);
        CHECK(tree.outer.nodes() == outer_before && tree.positions.nodes() == inner_before);
        verify_structure(tree, reference, 5);
    }

    mt19937 rng(0x2B17B17);
    for (nidx_t round = 0; round < 120; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 35), steps = 220;
        vector<nidx_t> reference(n);
        for (nidx_t& value : reference) value = nidx_t(rng() % 61) - 30;
        nrange_order_stat tree(reference, steps);
        set<nidx_t> historical(reference.begin(), reference.end());

        for (nidx_t step = 0; step < steps; ++step) {
            nidx_t type = 1 + nidx_t(rng() % 5);
            if (type == 3) {
                nidx_t position = nidx_t(rng() % n);
                nidx_t value = step % 37 == 0
                    ? (rng() & 1 ? -1'000'000'000 : 1'000'000'000)
                    : nidx_t(rng() % 61) - 30;
                nidx_t old = reference[position];
                nidx_t outer_before = tree.outer.nodes(), inner_before = tree.positions.nodes();
                nidx_t free_before = nidx_t(tree.free_nodes.size());
                tree.assign(position, value);
                reference[position] = value;
                historical.insert(value);
                if (old == value) {
                    CHECK(tree.outer.nodes() == outer_before);
                    CHECK(tree.positions.nodes() == inner_before);
                    CHECK(nidx_t(tree.free_nodes.size()) == free_before);
                } else {
                    CHECK(tree.outer.nodes() >= outer_before);
                    CHECK(tree.positions.nodes() >= inner_before);
                }
            } else {
                nidx_t left = nidx_t(rng() % n);
                nidx_t right = left + 1 + nidx_t(rng() % (n - left));
                vector<nidx_t> ordered = sorted_slice(reference, left, right);
                nidx_t outer_before = tree.outer.nodes(), inner_before = tree.positions.nodes();
                nidx_t free_before = nidx_t(tree.free_nodes.size());
                if (type == 1) {
                    nidx_t value = nidx_t(rng() % 81) - 40;
                    nidx_t expected = nidx_t(lower_bound(ordered.begin(), ordered.end(), value)
                                       - ordered.begin()) + 1;
                    CHECK(tree.rank(left, right, value) == expected);
                } else if (type == 2) {
                    nidx_t k = 1 + nidx_t(rng() % ordered.size());
                    CHECK(tree.kth(left, right, k) == ordered[k - 1]);
                } else if (type == 4) {
                    auto candidate = find_if(ordered.begin(), ordered.end(),
                        [](nidx_t value) { return value < 1'000'000'000; });
                    if (candidate == ordered.end()) {
                        CHECK(tree.kth(left, right, 1) == ordered[0]);
                        CHECK(tree.outer.nodes() == outer_before);
                        CHECK(tree.positions.nodes() == inner_before);
                        CHECK(nidx_t(tree.free_nodes.size()) == free_before);
                        continue;
                    }
                    nidx_t value = *candidate + 1;
                    auto it = lower_bound(ordered.begin(), ordered.end(), value);
                    CHECK(it != ordered.begin());
                    CHECK(tree.predecessor(left, right, value) == *prev(it));
                } else {
                    auto candidate = find_if(ordered.rbegin(), ordered.rend(),
                        [](nidx_t value) { return value > -1'000'000'000; });
                    if (candidate == ordered.rend()) {
                        CHECK(tree.kth(left, right, 1) == ordered[0]);
                        CHECK(tree.outer.nodes() == outer_before);
                        CHECK(tree.positions.nodes() == inner_before);
                        CHECK(nidx_t(tree.free_nodes.size()) == free_before);
                        continue;
                    }
                    nidx_t value = *candidate - 1;
                    auto it = upper_bound(ordered.begin(), ordered.end(), value);
                    CHECK(it != ordered.end());
                    CHECK(tree.successor(left, right, value) == *it);
                }
                CHECK(tree.outer.nodes() == outer_before);
                CHECK(tree.positions.nodes() == inner_before);
                CHECK(nidx_t(tree.free_nodes.size()) == free_before);
            }
            if (step % 53 == 0)
                verify_structure(tree, reference, nidx_t(historical.size()));
        }
        verify_structure(tree, reference, nidx_t(historical.size()));
    }
}
