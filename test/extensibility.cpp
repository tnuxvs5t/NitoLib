#include "../Nitori.h"

struct nsum_augment_test {
    using info_type = long long;
    long long scale = 1;
    int* ops = nullptr;

    info_type id() const { return 0; }
    info_type one(const int& x, int count) const { return scale * x * count; }
    info_type op(info_type a, info_type b) const {
        if (ops)
            ++*ops;
        return a + b;
    }
};

struct nstring_augment_test {
    using info_type = string;

    info_type id() const { return {}; }
    info_type one(const char& x, int count) const { return string(count, x); }
    info_type op(const info_type& a, const info_type& b) const { return a + b; }
};

using nsum_fhq_test = nset_fhq<int, nless<int>, true, nsum_augment_test>;
using nsum_splay_test = nset_splay<int, nless<int>, true, nsum_augment_test>;
using nstring_fhq_test = nset_fhq<char, nless<char>, true, nstring_augment_test>;
using nstring_splay_test = nset_splay<char, nless<char>, true, nstring_augment_test>;

static_assert(naugment<nsum_augment_test, int>);
static_assert(nnode_tree<nsum_fhq_test>);
static_assert(nnode_tree<nsum_splay_test>);
static_assert(naugmented_tree<nsum_fhq_test>);
static_assert(naugmented_tree<nsum_splay_test>);
static_assert(nnode_tree<nset<int>>);
static_assert(!nnode_tree<nset_stl<int>>);
static_assert(!naugmented_tree<nset_stl<int>>);
static_assert(same_as<nbag<int, nless<int>, nsum_augment_test>, nsum_fhq_test>);

template <class S> int tree_height(typename S::node_view u) {
    return u ? 1 + max(tree_height<S>(u.left()), tree_height<S>(u.right())) : 0;
}

template <class S> void check_sum_protocol() {
    int ops = 0;
    S s(nsum_augment_test{3, &ops});
    auto empty = s.root();
    assert(empty.current() && !empty && empty.len() == 0 && empty.info() == 0);

    s.ins(2, 3);
    s.ins(5, 2);
    s.ins(8);
    assert(s.len() == 6 && s.root().len() == 6);
    assert(s.root().info() == 3 * (2 * 3 + 5 * 2 + 8));

    vector<int> values{2, 2, 2, 5, 5, 8};
    nrep(i, int(values.size())) {
        int k = i;
        auto u = s.walk([&](auto v) {
            int left = v.left().len();
            if (k < left)
                return nbranch::left;
            if (k < left + v.count())
                return nbranch::take;
            k -= left + v.count();
            return nbranch::right;
        });
        assert(u && u.current() && u.val() == values[i]);
    }

    vector<pair<int, int>> blocks{{2, 6}, {5, 10}, {8, 8}};
    int total = 24;
    for (int target = 1; target <= total + 1; ++target) {
        int prefix = 0, want_first = npos;
        for (auto [x, sum] : blocks) {
            prefix += sum;
            if (prefix >= target) {
                want_first = x;
                break;
            }
        }
        int suffix = 0, want_last = npos;
        for (int i = int(blocks.size()); i--;) {
            suffix += blocks[i].second;
            if (suffix >= target) {
                want_last = blocks[i].first;
                break;
            }
        }
        auto first = s.first_prefix([&](long long x) { return x >= 3LL * target; });
        auto last = s.last_suffix([&](long long x) { return x >= 3LL * target; });
        assert(bool(first) == (want_first != npos));
        assert(bool(last) == (want_last != npos));
        if (first)
            assert(first.val() == want_first);
        if (last)
            assert(last.val() == want_last);
    }

    int height = tree_height<S>(s.root());
    ops = 0;
    auto middle = s.first_prefix([](long long x) { return x >= 36; });
    assert(middle && middle.val() == 5);
    assert(ops <= 2 * height);

    auto stale = s.root();
    s.del(2);
    assert(!stale.current() && !stale.ok());
    assert(s.root().info() == 3 * (2 * 2 + 5 * 2 + 8));

    auto assigned = s.root();
    S other(nless<int>{}, nsum_augment_test{3, &ops});
    other.ins(11, 2);
    s = other;
    assert(!assigned.current() && s.root().info() == 66);

    auto source = other.root();
    S moved(move(other));
    assert(!source.current() && moved.root().info() == 66);

    auto destination = s.root();
    auto moving = moved.root();
    s = move(moved);
    assert(!destination.current() && !moving.current() && s.root().info() == 66);
}

template <class S> void check_noncommutative_order() {
    S s;
    for (char x : {'d', 'a', 'c', 'b', 'b', 'f', 'e'})
        s.ins(x);
    assert(s.root().info() == "abbcdef");
    for (char x : {'a', 'f', 'c', 'b', 'e', 'd'}) {
        assert(s.has(x));
        assert(s.root().info() == "abbcdef");
    }
    assert(s.del('b') == 1);
    assert(s.root().info() == "abcdef");
}

template <class S> void check_prefix_differential() {
    S s(nless<int>{}, nsum_augment_test{});
    map<int, int> ref;
    mt19937 rng(712367);
    nrep(step, 2000) {
        int x = int(rng() % 30) + 1;
        int count = int(rng() % 4) + 1;
        if (rng() & 1) {
            assert(s.ins(x, count) == count);
            ref[x] += count;
        } else {
            int removed = min(count, ref[x]);
            assert(s.del(x, count) == removed);
            ref[x] -= removed;
            if (!ref[x])
                ref.erase(x);
        }

        long long total = 0;
        for (auto [value, multiplicity] : ref)
            total += 1LL * value * multiplicity;
        assert(s.root().info() == total);

        long long target = static_cast<long long>(rng() % (total + 4)) + 1;
        int want_first = npos;
        long long prefix = 0;
        for (auto [value, multiplicity] : ref) {
            prefix += 1LL * value * multiplicity;
            if (prefix >= target) {
                want_first = value;
                break;
            }
        }
        int want_last = npos;
        long long suffix = 0;
        for (auto i = ref.rbegin(); i != ref.rend(); ++i) {
            suffix += 1LL * i->first * i->second;
            if (suffix >= target) {
                want_last = i->first;
                break;
            }
        }
        auto first = s.first_prefix([&](long long value) { return value >= target; });
        auto last = s.last_suffix([&](long long value) { return value >= target; });
        assert(bool(first) == (want_first != npos));
        assert(bool(last) == (want_last != npos));
        if (first)
            assert(first.val() == want_first);
        if (last)
            assert(last.val() == want_last);
    }
}

void check_splay_view_expiry() {
    nsum_splay_test s;
    s.ins(2);
    s.ins(1);
    s.ins(3);
    auto old_root = s.root();
    assert(old_root.val() == 3 && s.has(1));
    assert(!old_root.current() && !old_root.ok());
}

void check_fhq_read_stability() {
    nsum_fhq_test s;
    s.ins(2);
    s.ins(1);
    s.ins(3);
    auto old_root = s.root();
    assert(s.has(1));
    assert(old_root.current() && old_root.ok());
}

int main() {
    nseed(20260721);
    check_sum_protocol<nsum_fhq_test>();
    check_sum_protocol<nsum_splay_test>();
    check_noncommutative_order<nstring_fhq_test>();
    check_noncommutative_order<nstring_splay_test>();
    check_prefix_differential<nsum_fhq_test>();
    check_prefix_differential<nsum_splay_test>();
    check_splay_view_expiry();
    check_fhq_read_stability();
}
