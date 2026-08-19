#include "../src-v3/segment.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    mt19937 rng(0xC0A3E7);
    for (nidx_t round = 0; round < 20000; ++round) {
        nidx_t n = nidx_t(rng() % 97);
        nidx_t base = nidx_t(bit_ceil(nuidx_t(max(nidx_t(1), n))));
        nidx_t left = nidx_t(rng() % (n + 1));
        nidx_t right = left + nidx_t(rng() % (n - left + 1));
        nidx_t cursor = left, pieces = 0;
        vector<nidx_t> covered(n);
        nsegment_cover(base, left, right,
                       [&](nidx_t node, nidx_t node_left, nidx_t node_right) {
            CHECK(node > 0 && node_left == cursor);
            CHECK(left <= node_left && node_left < node_right && node_right <= right);
            for (nidx_t i = node_left; i < node_right; ++i) ++covered[i];
            cursor = node_right;
            ++pieces;
        });
        CHECK(cursor == right);
        for (nidx_t i = 0; i < n; ++i)
            CHECK(covered[i] == (left <= i && i < right));
        nidx_t node_only_pieces = 0;
        nsegment_cover(base, left, right, [&](nidx_t) { ++node_only_pieces; });
        CHECK(node_only_pieces == pieces);

        if (n) {
            nidx_t position = nidx_t(rng() % n), expected_node = 1;
            nidx_t expected_left = 0, expected_right = base, depth = 0;
            nsegment_trace(base, position,
                           [&](nidx_t node, nidx_t node_left, nidx_t node_right) {
                CHECK(node == expected_node);
                CHECK(node_left == expected_left && node_right == expected_right);
                CHECK(node_left <= position && position < node_right);
                ++depth;
                if (node_left + 1 < node_right) {
                    nidx_t middle = midpoint(node_left, node_right);
                    nidx_t side = position < middle ? 0 : 1;
                    expected_node = expected_node * 2 + side;
                    if (side) expected_left = middle;
                    else expected_right = middle;
                }
            });
            CHECK(expected_left == position && expected_right == position + 1);
            CHECK(depth == bit_width(unsigned(base)));
        }
    }

    using outer_tree = nsparse_seg<multiset<nidx_t>, monostate>;
    constexpr long long lo = -128, hi = 128;
    outer_tree outer(lo, hi);
    nidx_t root = outer.make(multiset<nidx_t>{});
    vector<vector<nidx_t>> brute(hi - lo);

    auto open_child = [&](nidx_t node, nidx_t side) {
        nidx_t next = side ? outer[node].right : outer[node].left;
        if (next < 0) {
            next = outer.make(multiset<nidx_t>{});
            if (side) outer[node].right = next;
            else outer[node].left = next;
        }
        return next;
    };
    auto existing_child = [&](nidx_t node, nidx_t side) {
        return side ? outer[node].right : outer[node].left;
    };

    for (nidx_t round = 0; round < 30000; ++round) {
        if (rng() % 3) {
            long long position = lo + nidx_t(rng() % (hi - lo));
            nidx_t value = nidx_t(rng() % 1001) - 500;
            nidx_t visits = 0;
            nsegment_trace(root, lo, hi, position, open_child,
                           [&](nidx_t node, long long left, long long right) {
                CHECK(left <= position && position < right);
                outer[node].aggregate.insert(value);
                ++visits;
            });
            brute[position - lo].push_back(value);
            CHECK(visits == 9);
        } else {
            long long left = lo + nidx_t(rng() % (hi - lo + 1));
            long long right = left + nidx_t(rng() % (hi - left + 1));
            nidx_t value_left = nidx_t(rng() % 1101) - 550;
            nidx_t value_right = value_left + nidx_t(rng() % 101);
            nidx_t nodes_before = outer.nodes(), actual = 0;
            nsegment_cover(root, lo, hi, left, right, existing_child,
                           [&](nidx_t node, long long node_left, long long node_right) {
                CHECK(left <= node_left && node_left < node_right && node_right <= right);
                auto& values = outer[node].aggregate;
                actual += nidx_t(distance(values.lower_bound(value_left),
                                      values.lower_bound(value_right)));
            });
            nidx_t expected = 0;
            for (long long position = left; position < right; ++position)
                for (nidx_t value : brute[position - lo])
                    expected += value_left <= value && value < value_right;
            CHECK(actual == expected);
            CHECK(outer.nodes() == nodes_before);
        }
    }
    CHECK(outer.nodes() <= 2 * (hi - lo));

    nidx_t calls = 0;
    nsegment_trace(-1, lo, hi, 0LL,
                   [&](nidx_t, nidx_t) { ++calls; return -1; }, [&](nidx_t) { ++calls; });
    nsegment_cover(-1, lo, hi, -10LL, 10LL,
                   [&](nidx_t, nidx_t) { ++calls; return -1; }, [&](nidx_t) { ++calls; });
    CHECK(calls == 0);

    nsparse_seg<long long> ordinary(0, 8);
    nidx_t identity = ordinary.make();
    nidx_t left = ordinary.make(3), right = ordinary.make(4);
    nidx_t parent = ordinary.make(123, left, right);
    ordinary.pull(parent);
    CHECK(ordinary[identity].aggregate == 0);
    CHECK(ordinary[parent].aggregate == 7);
    CHECK(ordinary[parent].left == left && ordinary[parent].right == right);
}
