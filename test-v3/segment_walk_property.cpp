#include "../src-v3/segment.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    mt19937 rng(0xC0A3E7);
    for (int round = 0; round < 20000; ++round) {
        int n = int(rng() % 97);
        int base = int(bit_ceil(unsigned(max(1, n))));
        int left = int(rng() % (n + 1));
        int right = left + int(rng() % (n - left + 1));
        int cursor = left, pieces = 0;
        vector<int> covered(n);
        nsegment_cover(base, left, right,
                       [&](int node, int node_left, int node_right) {
            CHECK(node > 0 && node_left == cursor);
            CHECK(left <= node_left && node_left < node_right && node_right <= right);
            for (int i = node_left; i < node_right; ++i) ++covered[i];
            cursor = node_right;
            ++pieces;
        });
        CHECK(cursor == right);
        for (int i = 0; i < n; ++i)
            CHECK(covered[i] == (left <= i && i < right));
        int node_only_pieces = 0;
        nsegment_cover(base, left, right, [&](int) { ++node_only_pieces; });
        CHECK(node_only_pieces == pieces);

        if (n) {
            int position = int(rng() % n), expected_node = 1;
            int expected_left = 0, expected_right = base, depth = 0;
            nsegment_trace(base, position,
                           [&](int node, int node_left, int node_right) {
                CHECK(node == expected_node);
                CHECK(node_left == expected_left && node_right == expected_right);
                CHECK(node_left <= position && position < node_right);
                ++depth;
                if (node_left + 1 < node_right) {
                    int middle = midpoint(node_left, node_right);
                    int side = position < middle ? 0 : 1;
                    expected_node = expected_node * 2 + side;
                    if (side) expected_left = middle;
                    else expected_right = middle;
                }
            });
            CHECK(expected_left == position && expected_right == position + 1);
            CHECK(depth == bit_width(unsigned(base)));
        }
    }

    using outer_tree = nsparse_seg<multiset<int>, monostate>;
    constexpr long long lo = -128, hi = 128;
    outer_tree outer(lo, hi);
    int root = outer.make(multiset<int>{});
    vector<vector<int>> brute(hi - lo);

    auto open_child = [&](int node, int side) {
        int next = side ? outer[node].right : outer[node].left;
        if (next < 0) {
            next = outer.make(multiset<int>{});
            if (side) outer[node].right = next;
            else outer[node].left = next;
        }
        return next;
    };
    auto existing_child = [&](int node, int side) {
        return side ? outer[node].right : outer[node].left;
    };

    for (int round = 0; round < 30000; ++round) {
        if (rng() % 3) {
            long long position = lo + int(rng() % (hi - lo));
            int value = int(rng() % 1001) - 500;
            int visits = 0;
            nsegment_trace(root, lo, hi, position, open_child,
                           [&](int node, long long left, long long right) {
                CHECK(left <= position && position < right);
                outer[node].aggregate.insert(value);
                ++visits;
            });
            brute[position - lo].push_back(value);
            CHECK(visits == 9);
        } else {
            long long left = lo + int(rng() % (hi - lo + 1));
            long long right = left + int(rng() % (hi - left + 1));
            int value_left = int(rng() % 1101) - 550;
            int value_right = value_left + int(rng() % 101);
            int nodes_before = outer.nodes(), actual = 0;
            nsegment_cover(root, lo, hi, left, right, existing_child,
                           [&](int node, long long node_left, long long node_right) {
                CHECK(left <= node_left && node_left < node_right && node_right <= right);
                auto& values = outer[node].aggregate;
                actual += int(distance(values.lower_bound(value_left),
                                      values.lower_bound(value_right)));
            });
            int expected = 0;
            for (long long position = left; position < right; ++position)
                for (int value : brute[position - lo])
                    expected += value_left <= value && value < value_right;
            CHECK(actual == expected);
            CHECK(outer.nodes() == nodes_before);
        }
    }
    CHECK(outer.nodes() <= 2 * (hi - lo));

    int calls = 0;
    nsegment_trace(-1, lo, hi, 0LL,
                   [&](int, int) { ++calls; return -1; }, [&](int) { ++calls; });
    nsegment_cover(-1, lo, hi, -10LL, 10LL,
                   [&](int, int) { ++calls; return -1; }, [&](int) { ++calls; });
    CHECK(calls == 0);

    nsparse_seg<long long> ordinary(0, 8);
    int identity = ordinary.make();
    int left = ordinary.make(3), right = ordinary.make(4);
    int parent = ordinary.make(123, left, right);
    ordinary.pull(parent);
    CHECK(ordinary[identity].aggregate == 0);
    CHECK(ordinary[parent].aggregate == 7);
    CHECK(ordinary[parent].left == left && ordinary[parent].right == right);
}
