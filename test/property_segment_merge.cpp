#include "common.hpp"

struct nmerge_concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

static string random_word(mt19937& rng) {
    string result;
    int length = int(rng() % 3);
    for (int i = 0; i < length; ++i)
        result += char('a' + rng() % 4);
    return result;
}

static int test_fixed_ordered(mt19937& rng) {
    for (int trial = 0; trial < 300; ++trial) {
        int n = 1 + int(rng() % 32);
        nvector<string> left_values(n), right_values(n);
        vector<string> expected(static_cast<size_t>(n), string{});
        for (int i = 0; i < n; ++i) {
            left_values[i] = random_word(rng);
            right_values[i] = random_word(rng);
            expected[size_t(i)] = left_values[i] + right_values[i];
        }

        nseg<string, nmerge_concat> left(left_values), right(right_values);
        auto left_view = left.root();
        auto right_view = right.root();
        left.merge_from(move(right));
        ntest(!left_view.current() && !right_view.current());

        string all;
        for (const string& value : expected)
            all += value;
        ntest(left.fold() == all);
        for (int query = 0; query < 8; ++query) {
            int l = int(rng() % (n + 1)), r = int(rng() % (n + 1));
            if (l > r)
                swap(l, r);
            string range;
            for (int i = l; i < r; ++i)
                range += expected[size_t(i)];
            ntest(left.fold(l, r) == range);
        }
    }
    return 0;
}

static int test_fixed_lazy(mt19937& rng) {
    for (int trial = 0; trial < 250; ++trial) {
        int n = 1 + int(rng() % 32);
        nlazy_addsum<long long> left(n), right(n);
        vector<long long> left_ref(static_cast<size_t>(n), 0), right_ref(static_cast<size_t>(n), 0);
        for (int i = 0; i < n; ++i) {
            left_ref[size_t(i)] = int(rng() % 41) - 20;
            right_ref[size_t(i)] = int(rng() % 41) - 20;
            left.set(i, left_ref[size_t(i)]);
            right.set(i, right_ref[size_t(i)]);
        }
        for (int pass = 0; pass < 12; ++pass) {
            int l = int(rng() % (n + 1)), r = int(rng() % (n + 1));
            if (l > r)
                swap(l, r);
            long long delta = int(rng() % 31) - 15;
            if (rng() & 1) {
                left.apply(l, r, delta);
                for (int i = l; i < r; ++i)
                    left_ref[size_t(i)] += delta;
            } else {
                right.apply(l, r, delta);
                for (int i = l; i < r; ++i)
                    right_ref[size_t(i)] += delta;
            }
        }

        left.merge_from(move(right));
        for (int i = 0; i < n; ++i)
            ntest(left.get(i) == left_ref[size_t(i)] + right_ref[size_t(i)]);
        for (int query = 0; query < 8; ++query) {
            int l = int(rng() % (n + 1)), r = int(rng() % (n + 1));
            if (l > r)
                swap(l, r);
            long long expected = 0;
            for (int i = l; i < r; ++i)
                expected += left_ref[size_t(i)] + right_ref[size_t(i)];
            ntest(left.fold(l, r) == expected);
        }
    }
    return 0;
}

static int test_dynamic_ordered(mt19937& rng) {
    using tree_type = ndynamic_seg<string, nmerge_concat>;
    constexpr int lo = -40, hi = 40, width = hi - lo;
    for (int trial = 0; trial < 300; ++trial) {
        auto domain = tree_type(lo, hi).domain();
        tree_type left(domain, lo, hi), right(domain, lo, hi);
        vector<string> left_ref(static_cast<size_t>(width), string{}),
            right_ref(static_cast<size_t>(width), string{});
        for (int pass = 0; pass < 20; ++pass) {
            int index = int(rng() % width);
            if (rng() & 1) {
                string value = random_word(rng);
                left.set(lo + index, value);
                left_ref[size_t(index)] = move(value);
            } else {
                string value = random_word(rng);
                right.set(lo + index, value);
                right_ref[size_t(index)] = move(value);
            }
        }

        left.merge_from(move(right));
        vector<string> expected(static_cast<size_t>(width), string{});
        for (int i = 0; i < width; ++i)
            expected[size_t(i)] = left_ref[size_t(i)] + right_ref[size_t(i)];
        for (int query = 0; query < 12; ++query) {
            int l = int(rng() % (width + 1)), r = int(rng() % (width + 1));
            if (l > r)
                swap(l, r);
            string actual, reference;
            for (int i = l; i < r; ++i) {
                actual += left.get(lo + i);
                reference += expected[size_t(i)];
            }
            ntest(actual == reference);
            ntest(left.fold(lo + l, lo + r) == reference);
        }
    }
    return 0;
}

static int test_dynamic_lazy(mt19937& rng) {
    using tree_type = ndynamic_addsum<long long>;
    constexpr int lo = -32, hi = 32, width = hi - lo;
    for (int trial = 0; trial < 250; ++trial) {
        auto domain = tree_type(lo, hi).domain();
        tree_type left(domain, lo, hi), right(domain, lo, hi);
        vector<long long> left_ref(static_cast<size_t>(width), 0),
            right_ref(static_cast<size_t>(width), 0);
        for (int pass = 0; pass < 30; ++pass) {
            int l = int(rng() % (width + 1)), r = int(rng() % (width + 1));
            if (l > r)
                swap(l, r);
            if (rng() % 3 == 0) {
                int index = int(rng() % width);
                long long value = int(rng() % 101) - 50;
                if (rng() & 1) {
                    left.set(lo + index, value);
                    left_ref[size_t(index)] = value;
                } else {
                    right.set(lo + index, value);
                    right_ref[size_t(index)] = value;
                }
            } else {
                long long delta = int(rng() % 31) - 15;
                if (rng() & 1) {
                    left.apply(lo + l, lo + r, delta);
                    for (int i = l; i < r; ++i)
                        left_ref[size_t(i)] += delta;
                } else {
                    right.apply(lo + l, lo + r, delta);
                    for (int i = l; i < r; ++i)
                        right_ref[size_t(i)] += delta;
                }
            }
        }

        int nodes_before_query = left.nodes() + right.nodes();
        ntest(left.fold() == accumulate(left_ref.begin(), left_ref.end(), 0LL));
        ntest(right.fold() == accumulate(right_ref.begin(), right_ref.end(), 0LL));
        ntest(left.nodes() + right.nodes() == nodes_before_query);

        left.merge_from(move(right));
        for (int query = 0; query < 12; ++query) {
            int l = int(rng() % (width + 1)), r = int(rng() % (width + 1));
            if (l > r)
                swap(l, r);
            long long expected = 0;
            for (int i = l; i < r; ++i)
                expected += left_ref[size_t(i)] + right_ref[size_t(i)];
            ntest(left.fold(lo + l, lo + r) == expected);
        }
    }
    return 0;
}

static int test_persistent_ordered(mt19937& rng) {
    using tree_type = npersistent_seg<string, nmerge_concat>;
    for (int trial = 0; trial < 20; ++trial) {
        int n = 1 + int(rng() % 24);
        nvector<string> initial(n);
        vector<vector<string>> references(1, vector<string>(size_t(n)));
        for (int i = 0; i < n; ++i)
            references[0][size_t(i)] = initial[i] = random_word(rng);
        tree_type tree(initial, nmerge_concat{});

        for (int operation = 0; operation < 120; ++operation) {
            if (rng() % 3) {
                int base = int(rng() % references.size());
                int index = int(rng() % n);
                string value = random_word(rng);
                int created = tree.set(base, index, value);
                auto snapshot = references[size_t(base)];
                snapshot[size_t(index)] = move(value);
                references.push_back(move(snapshot));
                ntest(created == int(references.size()) - 1);
            } else {
                int left_version = int(rng() % references.size());
                int right_version = int(rng() % references.size());
                int created = tree.merge(left_version, right_version);
                vector<string> snapshot(static_cast<size_t>(n), string{});
                for (int i = 0; i < n; ++i)
                    snapshot[size_t(i)] = references[size_t(left_version)][size_t(i)] +
                                           references[size_t(right_version)][size_t(i)];
                references.push_back(move(snapshot));
                ntest(created == int(references.size()) - 1);
            }
            for (int query = 0; query < 3; ++query) {
                int version = int(rng() % references.size());
                int l = int(rng() % (n + 1)), r = int(rng() % (n + 1));
                if (l > r)
                    swap(l, r);
                string expected;
                for (int i = l; i < r; ++i)
                    expected += references[size_t(version)][size_t(i)];
                ntest(tree.fold(version, l, r) == expected);
            }
        }
    }
    return 0;
}

int main() {
    mt19937 rng(0x9e3779b9U);
    ntest(test_fixed_ordered(rng) == 0);
    ntest(test_fixed_lazy(rng) == 0);
    ntest(test_dynamic_ordered(rng) == 0);
    ntest(test_dynamic_lazy(rng) == 0);
    ntest(test_persistent_ordered(rng) == 0);
}
