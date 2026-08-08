#include "common.hpp"

struct naffine_tag_ext {
    long long multiply, add;
};

struct naffine_action_ext {
    naffine_tag_ext tag_id() const { return {1, 0}; }
    naffine_tag_ext compose(const naffine_tag_ext& newer, const naffine_tag_ext& older) const {
        return {newer.multiply * older.multiply, newer.multiply * older.add + newer.add};
    }
    long long apply(long long sum, const naffine_tag_ext& tag, int length) const {
        return tag.multiply * sum + tag.add * length;
    }
};

struct nstring_concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

struct nsum_augment {
    using info_type = long long;
    long long id() const { return 0; }
    long long one(const long long& value, int count) const { return value * count; }
    long long op(long long left, long long right) const { return left + right; }
};

struct nfhq_add_tag {
    using tag_type = int;
    int tag_id() const { return 0; }
    int compose(int newer, int older) const { return newer + older; }
    long long apply_value(long long value, int tag, int) const { return value + tag; }
    long long apply_info(long long info, int tag, int length) const {
        return info + 1LL * tag * length;
    }
};

int main() {
    ndynamic_seg<long long> point(-1000000000000LL, 1000000000000LL);
    point.set(3, 7);
    point.combine(3, 5);
    point.set(-9, 2);
    ntest(point.fold(-10, 4) == 14);
    ntest(point.get(3) == 12 && point.get(4) == 0);
    ntest(point.root().left_bound() == -1000000000000LL);
    auto path = nseg_walk(point, [](auto node) {
        if (node.leaf())
            return nbranch::take;
        long long middle = node.left_bound() + node.width() / 2;
        return 3 < middle ? nbranch::left : nbranch::right;
    });
    ntest(path.leaf());

    ndynamic_seg<string, nstring_concat> text(-10, 10);
    text.set(-2, "a");
    text.set(4, "b");
    text.combine(-2, "x");
    ntest(text.fold(-10, 10) == "axb");

    ndynamic_addsum<long long> lazy(-8, 8);
    lazy.apply(-3, 5, 4);
    lazy.apply(0, 8, 2);
    lazy.set(-8, 10);
    int nodes_before_query = lazy.nodes();
    ntest(lazy.fold(-8, 8) == 58);
    ntest(lazy.fold(-3, 0) == 12 && lazy.get(7) == 2);
    ntest(lazy.nodes() == nodes_before_query);
    ntest(lazy.root().tag() == 0);

    ndynamic_lazyseg<long long, naffine_tag_ext, nadd<long long>, naffine_action_ext> affine(0, 8);
    for (int i = 0; i < 8; ++i)
        affine.set(i, i + 1);
    affine.apply(0, 8, {2, 1});
    affine.apply(0, 8, {3, 4});
    int affine_nodes = affine.nodes();
    ntest(affine.fold() == 272 && affine.fold(2, 5) == 93);
    ntest(affine.root().left().aggregate() == 88);
    ntest(affine.nodes() == affine_nodes);

    nlazy_addsum<long long> fixed_lazy(8);
    fixed_lazy.apply(0, 8, 3);
    ntest(fixed_lazy.root().aggregate() == 24 && fixed_lazy.root().tag() == 3);
    auto fixed_child = fixed_lazy.root().left();
    ntest(fixed_child.aggregate() == 12 && fixed_lazy.root().right().aggregate() == 12);
    ntest(fixed_lazy.fold(0, 1) == 3);
    ntest(fixed_child.current() && fixed_child.aggregate() == 12);

    ndynamic_addsum<long long> dynamic_view(0, 8);
    dynamic_view.apply(0, 8, 3);
    int dynamic_view_nodes = dynamic_view.nodes();
    auto implicit_child = dynamic_view.root().left();
    ntest(!implicit_child && implicit_child.aggregate() == 12);
    ntest(dynamic_view.nodes() == dynamic_view_nodes);

    nvector<int> initial{1, 2, 3, 4};
    nseg<int> iterative(initial);
    ntest(iterative.root().aggregate() == 10);
    auto old_root = iterative.root();
    iterative.set(0, 9);
    ntest(!old_root.current() && iterative.root().aggregate() == 18);
    nseg<int> assigned(4);
    auto assigned_root = assigned.root();
    assigned = iterative;
    ntest(!assigned_root.current() && assigned.root().aggregate() == 18);

    npersistent_seg<int> persistent(initial);
    auto stable_version = persistent.root(0);
    int version = persistent.set(0, 1, 20);
    ntest(stable_version.current());
    ntest(persistent.root(version).aggregate() == 28);
    ntest(persistent.root(0).right().left().aggregate() == 3);
    npersistent_seg<int> persistent_target(4);
    auto replaced_version = persistent_target.root(0);
    persistent_target = persistent;
    ntest(!replaced_version.current() && persistent_target.root(version).aggregate() == 28);

    using tagged_bag = nset_fhq<long long, nless<long long>, true, nsum_augment, nfhq_add_tag>;
    tagged_bag bag(nless<long long>{}, nsum_augment{}, nfhq_add_tag{});
    bag.ins(1, 2);
    bag.ins(5);
    auto selected = bag.root();
    bag.apply(selected, 3);
    ntest(bag.root().tag() == 3);
    ntest(bag.root().info() == 16);
    auto child = bag.root().left();
    if (!child)
        child = bag.root().right();
    int child_value = int(child.val()), child_len = child.len();
    bag.apply(child, 1);
    ntest(bag.root().info() == 16 + child_len);
    ntest(bag.has(child_value + 1));
    ntest(bag.root().tag() == 0);
}
