#include "common.hpp"

struct nconcat_segment_domain {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

static int test_point_merge() {
    using tree_type = ndynamic_seg<string, nconcat_segment_domain>;
    auto domain = tree_type(-8, 8).domain();
    tree_type left(domain, -8, 8), right(domain, -8, 8);
    left.set(4, "a");
    right.set(-3, "b");
    right.set(6, "c");

    auto left_before = left.root();
    auto right_before = right.root();
    ntest(left.root().same_domain(right.root()));
    left.merge_from(move(right));
    ntest(right.empty() && !left_before.current() && !right_before.current());
    ntest(left.get(-3) == "b" && left.get(4) == "a" && left.get(6) == "c");
    // Pointwise merging must preserve coordinate order.  An aggregate-level
    // `left.fold() op right.fold()` would produce "abc" for this arrangement.
    ntest(left.fold() == "bac");
    auto identity = left.root().identity();
    ntest(identity && identity.domain && identity.handle > 0 && identity.generation > 0);

    tree_type copy = left;
    ntest(!copy.same_domain(left));
    copy.combine(4, "x");
    ntest(left.get(4) == "a" && copy.get(4) == "ax");
    return 0;
}

static int test_lazy_merge() {
    using tree_type = ndynamic_addsum<long long>;
    auto domain = tree_type(0, 8).domain();
    tree_type left(domain, 0, 8), right(domain, 0, 8);
    left.apply(0, 8, 2);
    left.set(1, 5);
    right.apply(0, 4, 3);
    right.set(2, 7);

    auto left_before = left.root();
    auto right_before = right.root();
    int nodes_before_query = left.nodes();
    ntest(left.fold(0, 8) == 19 && right.fold(0, 8) == 16);
    ntest(left.nodes() == nodes_before_query);

    left.merge_from(move(right));
    ntest(right.empty() && !left_before.current() && !right_before.current());
    ntest(left.fold() == 35);
    ntest(left.get(0) == 5 && left.get(1) == 8 && left.get(2) == 9 && left.get(3) == 5 &&
          left.get(4) == 2 && left.get(7) == 2);
    ntest(left.root().aggregate() == 35);
    auto identity = left.root().identity();
    ntest(identity && identity.handle > 0);

    auto sibling_domain = tree_type(0, 8).domain();
    tree_type clearer(sibling_domain, 0, 8), survivor(sibling_domain, 0, 8);
    clearer.set(1, 4);
    survivor.set(2, 9);
    auto survivor_view = survivor.root();
    clearer.clear();
    ntest(!survivor_view.current() && survivor.get(2) == 9);
    return 0;
}

static int test_fixed_merge() {
    nvector<string> left_values(4), right_values(4);
    left_values[3] = "a";
    right_values[0] = "b";
    nseg<string, nconcat_segment_domain> left(left_values), right(right_values);
    auto left_view = left.root();
    auto right_view = right.root();
    left.merge_from(move(right));
    ntest(left.fold() == "ba" && right.fold().empty());
    ntest(!left_view.current() && !right_view.current());

    nlazy_addsum<long long> lazy_left(4), lazy_right(4);
    lazy_left.apply(0, 4, 2);
    lazy_right.apply(1, 3, 5);
    lazy_left.merge_from(move(lazy_right));
    ntest(lazy_left.fold() == 18 && lazy_left.get(0) == 2 && lazy_left.get(1) == 7 &&
          lazy_left.get(2) == 7 && lazy_left.get(3) == 2 && lazy_right.fold() == 0);
    return 0;
}

int main() {
    ntest(test_point_merge() == 0);
    ntest(test_lazy_merge() == 0);
    ntest(test_fixed_merge() == 0);

    nvector<int> source{1, 2, 3, 4};
    npersistent_seg<int> persistent(source);
    auto old = persistent.root(0);
    int first = persistent.set(0, 0, 10);
    int second = persistent.set(0, 2, 20);
    int merged = persistent.merge(first, second);
    ntest(old.current());
    ntest(persistent.fold(merged) == 46);
    ntest(persistent.fold(0) == 10 && persistent.fold(first) == 19 && persistent.fold(second) == 27);
    ntest(persistent.root(merged).identity().handle > 0);
}
