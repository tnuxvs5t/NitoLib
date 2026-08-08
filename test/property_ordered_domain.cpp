#include "common.hpp"

template <class S> static vector<int> ordered_values(const S& tree) {
    vector<int> result;
    nfor(value, tree)
        result.push_back(value);
    return result;
}

template <class S, bool Multi> static int check_ordered_domain(uint32_t seed) {
    using reference_type = conditional_t<Multi, multiset<int>, set<int>>;
    mt19937 random(seed);

    auto domain = S{}.domain();
    S left(domain), right(domain);
    for (int value : {1, 3, 5})
        left.ins(value);
    for (int value : {7, 9})
        right.ins(value);

    auto left_before = left.root();
    auto right_before = right.root();
    ntest(left.same_domain(right) && left.root().same_domain(right.root()));
    left.merge_from(move(right));
    ntest(right.empty());
    ntest(!left_before.current() && !right_before.current());
    ntest(ordered_values(left) == vector<int>({1, 3, 5, 7, 9}));

    auto pieces = move(left).split_by(6);
    ntest(pieces.first.same_domain(pieces.second));
    ntest(ordered_values(pieces.first) == vector<int>({1, 3, 5}));
    ntest(ordered_values(pieces.second) == vector<int>({7, 9}));
    auto split_view = pieces.first.root();
    pieces.first.merge_from(move(pieces.second));
    ntest(!split_view.current());
    ntest(ordered_values(pieces.first) == vector<int>({1, 3, 5, 7, 9}));
    auto identity = pieces.first.root().identity();
    ntest(identity && identity.domain && identity.handle > 0 && identity.generation > 0);

    S copy = pieces.first;
    ntest(!copy.same_domain(pieces.first));
    copy.ins(11);
    ntest(ordered_values(pieces.first) == vector<int>({1, 3, 5, 7, 9}));
    ntest(ordered_values(copy) == vector<int>({1, 3, 5, 7, 9, 11}));

    auto sibling_domain = S{}.domain();
    S clearer(sibling_domain), survivor(sibling_domain);
    clearer.ins(17);
    survivor.ins(19);
    auto survivor_view = survivor.root();
    clearer.clear();
    ntest(survivor_view.current() == false && survivor.root().ok() && survivor.root().val() == 19);

    reference_type reference;
    S tree;
    for (int step = 0; step < 1800; ++step) {
        int value = int(random() % 80);
        int count = 1 + int(random() % 3);
        if (random() & 1) {
            tree.ins(value, count);
            if constexpr (Multi)
                for (int i = 0; i < count; ++i)
                    reference.insert(value);
            else
                reference.insert(value);
        } else {
            tree.del(value, count);
            if constexpr (Multi) {
                for (int i = 0; i < count; ++i) {
                    auto found = reference.find(value);
                    if (found == reference.end())
                        break;
                    reference.erase(found);
                }
            } else {
                reference.erase(value);
            }
        }

        int cut = int(random() % 80);
        auto split = move(tree).split_by(cut);
        vector<int> expected_left, expected_right;
        for (int x : reference)
            (x < cut ? expected_left : expected_right).push_back(x);
        ntest(ordered_values(split.first) == expected_left);
        ntest(ordered_values(split.second) == expected_right);
        split.first.merge_from(move(split.second));
        tree = move(split.first);
        ntest(ordered_values(tree) == vector<int>(reference.begin(), reference.end()));
    }
    return 0;
}

int main() {
    ntest((check_ordered_domain<nset_fhq<int>, false>(0x2305U) == 0));
    ntest((check_ordered_domain<nset_splay<int>, false>(0x2306U) == 0));
    ntest((check_ordered_domain<nbag<int>, true>(0x2307U) == 0));
    ntest((check_ordered_domain<nset_splay<int, nless<int>, true>, true>(0x2308U) == 0));
}
