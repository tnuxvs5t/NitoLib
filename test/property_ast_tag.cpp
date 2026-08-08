#include "common.hpp"

struct ntag_sum_augment {
    using info_type = long long;
    long long id() const { return 0; }
    long long one(const int& value, int count) const { return 1LL * value * count; }
    long long op(long long left, long long right) const { return left + right; }
};

struct ntag_add_action {
    using tag_type = int;
    int tag_id() const { return 0; }
    int compose(int newer, int older) const { return newer + older; }
    int apply_value(int value, int tag, int) const { return value + tag; }
    long long apply_info(long long info, int tag, int length) const {
        return info + 1LL * tag * length;
    }
};

template <>
inline constexpr bool nnode_action_laws<ntag_add_action, int, long long> = true;

using tagged_tree =
    nset_fhq<int, nless<int>, true, ntag_sum_augment, ntag_add_action>;

static void collect_subtree(tagged_tree::node_view node, vector<pair<int, int>>& values) {
    if (!node)
        return;
    collect_subtree(node.left(), values);
    values.push_back({node.val(), node.count()});
    collect_subtree(node.right(), values);
}

int main() {
    nseed(0x23a611U);
    mt19937 rng(0x23a612U);
    tagged_tree tree;
    map<int, int> reference;

    for (int step = 0; step < 3000; ++step) {
        int kind = int(rng() % 5);
        if (kind == 0 || reference.empty()) {
            int value = int(rng() % 101) - 50;
            int count = 1 + int(rng() % 3);
            tree.ins(value, count);
            reference[value] += count;
        } else if (kind == 1) {
            int value = int(rng() % 101) - 50;
            int count = 1 + int(rng() % 3);
            int removed = min(count, reference[value]);
            ntest(tree.del(value, count) == removed);
            if ((reference[value] -= removed) == 0)
                reference.erase(value);
        } else if (kind == 2) {
            int delta = int(rng() % 7) - 3;
            tree.apply(delta);
            map<int, int> shifted;
            for (auto [value, count] : reference)
                shifted[value + delta] += count;
            reference = move(shifted);
        } else if (kind == 3 && tree.len() > tree.root().count()) {
            auto root = tree.root();
            auto selected = root.left();
            int delta = -1;
            if (!selected) {
                selected = root.right();
                delta = 1;
            }
            vector<pair<int, int>> changed;
            collect_subtree(selected, changed);
            tree.apply(selected, delta);
            for (auto [value, count] : changed) {
                reference[value] -= count;
                if (!reference[value])
                    reference.erase(value);
                reference[value + delta] += count;
            }
        }

        int expected_len = 0;
        long long expected_sum = 0;
        vector<int> expected_values;
        for (auto [value, count] : reference) {
            expected_len += count;
            expected_sum += 1LL * value * count;
            expected_values.insert(expected_values.end(), size_t(count), value);
        }
        ntest(tree.len() == expected_len && tree.root().info() == expected_sum);
        vector<int> actual;
        nfor(value, tree)
            actual.push_back(value);
        ntest(actual == expected_values);
    }
}
