#include "common.hpp"

struct run_info {
    int len = 0, first = 0, last = 0, prefix = 0, suffix = 0, best = 0;
};

static run_info join_runs(run_info left, run_info right) {
    if (!left.len)
        return right;
    if (!right.len)
        return left;
    run_info result;
    result.len = left.len + right.len;
    result.first = left.first;
    result.last = right.last;
    result.prefix = left.prefix == left.len && left.last == right.first
                         ? left.len + right.prefix
                         : left.prefix;
    result.suffix = right.suffix == right.len && left.last == right.first
                        ? right.len + left.suffix
                        : right.suffix;
    result.best = max(left.best, right.best);
    if (left.last == right.first) {
        result.best = max(result.best, left.suffix + right.prefix);
    }
    return result;
}

struct run_assign {
    int value;
};
struct run_reverse {};

struct run_policy {
    using info_type = run_info;
    struct state_type {
        bool assigned = false;
        int value = 0;
        bool reversed = false;
    };

    info_type id() const { return {}; }
    info_type leaf(int value) const { return {1, value, value, 1, 1, 1}; }
    state_type state_id() const { return {}; }

    void pull(auto node) const {
        run_info left = node.left() ? node.left().info() : run_info{};
        run_info self = leaf(node.val());
        run_info right = node.right() ? node.right().info() : run_info{};
        node.info() = join_runs(join_runs(left, self), right);
    }
    void apply(auto node, run_assign tag) const {
        node.val() = tag.value;
        node.info() = {node.len(), tag.value, tag.value, node.len(), node.len(), node.len()};
        auto& state = node.state();
        state.assigned = true;
        state.value = tag.value;
    }
    void apply(auto node, run_reverse) const {
        node.exchange_children();
        swap(node.info().first, node.info().last);
        swap(node.info().prefix, node.info().suffix);
        node.state().reversed = !node.state().reversed;
    }
    void push(auto node) const {
        state_type state = node.state();
        if (state.reversed) {
            if (node.left())
                node.left().apply(run_reverse{});
            if (node.right())
                node.right().apply(run_reverse{});
        }
        if (state.assigned) {
            if (node.left())
                node.left().apply(run_assign{state.value});
            if (node.right())
                node.right().apply(run_assign{state.value});
        }
        node.state() = state_id();
    }
};

struct implicit_sum_augment {
    using info_type = long long;
    long long id() const { return 0; }
    long long one(const int& value, int count) const { return 1LL * value * count; }
    long long op(long long left, long long right) const { return left + right; }
};

struct implicit_add_action {
    using tag_type = int;
    int tag_id() const { return 0; }
    int compose(int newer, int older) const { return newer + older; }
    int apply_value(int value, int tag, int) const { return value + tag; }
    long long apply_info(long long info, int tag, int length) const {
        return info + 1LL * tag * length;
    }
};

static vector<int> values_of(const nimplicit_fhq<int, run_policy>& tree) {
    vector<int> values;
    nfor(value, tree)
        values.push_back(value);
    return values;
}

int main() {
    nseed(0x2311U);

    using sum_policy = nfhq_policy<int, implicit_sum_augment, implicit_add_action>;
    nimplicit_fhq<int, sum_policy> sums({1, 2, 3, 4}, sum_policy{});
    sums.apply(1, 4, 5);
    ntest(sums.fold() == 25 && sums.fold(1, 3) == 15);
    ntest(values_of(nimplicit_fhq<int, run_policy>{}) == vector<int>{});

    nimplicit_fhq<int, run_policy> tree{0, 1, 1, 0, 1, 0};
    ntest(values_of(tree) == vector<int>({0, 1, 1, 0, 1, 0}));
    tree.apply(1, 5, run_reverse{});
    ntest(values_of(tree) == vector<int>({0, 1, 0, 1, 1, 0}));
    tree.apply(2, 5, run_assign{0});
    ntest(values_of(tree) == vector<int>({0, 1, 0, 0, 0, 0}));
    ntest(tree.fold().best == 4);
    tree.splice(1, 4, 3);
    ntest(values_of(tree) == vector<int>({0, 0, 0, 1, 0, 0}));
    tree.rotate(1, 4, 6);
    ntest(values_of(tree) == vector<int>({0, 0, 0, 0, 0, 1}));
    tree.ins(2, 1);
    tree.del(3);
    ntest(values_of(tree) == vector<int>({0, 0, 1, 0, 0, 1}));

    auto root = tree.root();
    auto child = root.left();
    if (!child)
        child = root.right();
    ntest(child && child.parent() && child.parent().same_owner(root));
    auto selected = nwalk(child, [](auto node) {
        if (node.leaf())
            return nbranch::take;
        return node.left() ? nbranch::left : nbranch::right;
    });
    ntest(selected && selected.same_owner(root));

    mt19937 rng(0x2312U);
    vector<int> reference = values_of(tree);
    for (int step = 0; step < 1200; ++step) {
        int kind = int(rng() % 6);
        if (kind == 0 || reference.empty()) {
            int at = int(rng() % (reference.size() + 1));
            int value = int(rng() & 1);
            tree.ins(at, value);
            reference.insert(reference.begin() + at, value);
        } else if (kind == 1) {
            int at = int(rng() % reference.size());
            tree.set(at, int(rng() & 1));
            reference[at] = tree.get(at);
        } else if (kind == 2) {
            int l = int(rng() % (reference.size() + 1));
            int r = l + int(rng() % (reference.size() - l + 1));
            tree.apply(l, r, run_reverse{});
            reverse(reference.begin() + l, reference.begin() + r);
        } else if (kind == 3) {
            int l = int(rng() % (reference.size() + 1));
            int r = l + int(rng() % (reference.size() - l + 1));
            int value = int(rng() & 1);
            tree.apply(l, r, run_assign{value});
            fill(reference.begin() + l, reference.begin() + r, value);
        } else if (kind == 4 && reference.size() > 1) {
            int l = int(rng() % reference.size());
            int r = l + int(rng() % (reference.size() - l + 1));
            int width = r - l;
            int at = int(rng() % (reference.size() - width + 1));
            tree.splice(l, r, at);
            vector<int> block(reference.begin() + l, reference.begin() + r);
            reference.erase(reference.begin() + l, reference.begin() + r);
            reference.insert(reference.begin() + at, block.begin(), block.end());
        } else {
            int l = int(rng() % reference.size());
            int r = l + 1 + int(rng() % (reference.size() - l));
            tree.del(l, r);
            reference.erase(reference.begin() + l, reference.begin() + r);
        }
        ntest(values_of(tree) == reference);
        int best = 0, current = 0, previous = -1;
        for (int value : reference) {
            current = value == previous ? current + 1 : 1;
            previous = value;
            best = max(best, current);
        }
        ntest(tree.fold().best == best);
        if (!reference.empty() && step % 7 == 0) {
            int l = int(rng() % reference.size());
            int r = l + 1 + int(rng() % (reference.size() - l));
            run_info expected;
            for (int i = l; i < r; ++i)
                expected = join_runs(expected, run_policy{}.leaf(reference[i]));
            ntest(tree.fold(l, r).best == expected.best);
        }
    }

    auto shared_domain = nseq_fhq<int>{}.domain();
    nseq_fhq<int> left(shared_domain), right(shared_domain);
    auto plain_values = [](const nseq_fhq<int>& sequence) {
        vector<int> result;
        nfor(value, sequence)
            result.push_back(value);
        return result;
    };
    left.push(1);
    left.push(2);
    right.push(3);
    right.push(4);
    auto left_before_merge = left.root();
    auto right_before_merge = right.root();
    ntest(left.same_domain(right) && left.root().same_domain(right.root()));
    left.merge_from(move(right));
    ntest(plain_values(left) == vector<int>({1, 2, 3, 4}) && right.empty());
    ntest(!left_before_merge.current() && !right_before_merge.current());

    auto pieces = move(left).split_at(2);
    ntest(pieces.first.same_domain(pieces.second));
    ntest(plain_values(pieces.first) == vector<int>({1, 2}));
    ntest(plain_values(pieces.second) == vector<int>({3, 4}));
    pieces.first.merge_from(move(pieces.second));
    ntest(plain_values(pieces.first) == vector<int>({1, 2, 3, 4}) && pieces.second.empty());
}
