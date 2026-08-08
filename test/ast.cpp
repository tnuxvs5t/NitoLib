#include "common.hpp"

struct sum_augment {
    using info_type = long long;
    long long scale = 1;

    info_type id() const { return 0; }
    info_type one(const int& value, int count) const { return scale * value * count; }
    info_type op(info_type left, info_type right) const { return left + right; }
};

struct string_augment {
    using info_type = string;
    info_type id() const { return {}; }
    info_type one(const char& value, int count) const { return string(size_t(count), value); }
    info_type op(const info_type& left, const info_type& right) const { return left + right; }
};

using sum_fhq = nset_fhq<int, nless<int>, true, sum_augment>;
using sum_splay = nset_splay<int, nless<int>, true, sum_augment>;

template <class S> int check_ast() {
    S tree(nless<int>{}, sum_augment{3});
    auto empty = tree.root();
    ntest(empty.current() && !empty && empty.len() == 0 && empty.info() == 0);
    tree.ins(2, 3);
    tree.ins(5, 2);
    tree.ins(8);
    ntest(tree.root().len() == 6);
    ntest(tree.root().info() == 3 * (2 * 3 + 5 * 2 + 8));

    nvector<int> values{2, 2, 2, 5, 5, 8};
    for (int index = 0; index < values.len(); ++index) {
        int remaining = index;
        auto found = tree.walk([&](auto node) {
            int left = node.left().len();
            if (remaining < left)
                return nbranch::left;
            if (remaining < left + node.count())
                return nbranch::take;
            remaining -= left + node.count();
            return nbranch::right;
        });
        ntest(found && found.val() == values[index]);
    }

    for (long long target = 1; target <= 25; ++target) {
        int expected_first = target <= 6 ? 2 : target <= 16 ? 5 : target <= 24 ? 8 : npos;
        int expected_last = target <= 8 ? 8 : target <= 18 ? 5 : target <= 24 ? 2 : npos;
        auto first = tree.first_prefix([&](long long aggregate) { return aggregate >= 3 * target; });
        auto last = tree.last_suffix([&](long long aggregate) { return aggregate >= 3 * target; });
        ntest(bool(first) == (expected_first != npos));
        ntest(bool(last) == (expected_last != npos));
        if (first)
            ntest(first.val() == expected_first);
        if (last)
            ntest(last.val() == expected_last);
    }

    auto stale = tree.root();
    tree.del(2);
    ntest(!stale.current() && !stale.ok());
    return 0;
}

template <class S> int differential_ast(uint32_t seed) {
    S tree;
    map<int, int> reference;
    mt19937 random(seed);
    for (int step = 0; step < 3000; ++step) {
        int value = 1 + int(random() % 40), count = 1 + int(random() % 4);
        if (random() & 1) {
            ntest(tree.ins(value, count) == count);
            reference[value] += count;
        } else {
            int removed = min(count, reference[value]);
            ntest(tree.del(value, count) == removed);
            reference[value] -= removed;
            if (!reference[value])
                reference.erase(value);
        }
        long long total = 0;
        for (auto [key, multiplicity] : reference)
            total += 1LL * key * multiplicity;
        ntest(tree.root().info() == total);
        if (total) {
            long long target = 1 + random() % total;
            int expected = npos;
            long long prefix = 0;
            for (auto [key, multiplicity] : reference) {
                prefix += 1LL * key * multiplicity;
                if (prefix >= target) {
                    expected = key;
                    break;
                }
            }
            auto found = tree.first_prefix([&](long long aggregate) { return aggregate >= target; });
            ntest(found && found.val() == expected);
        }
    }
    return 0;
}

int main() {
    nseed(0x0501U);
    ntest(check_ast<sum_fhq>() == 0);
    ntest(check_ast<sum_splay>() == 0);
    ntest((differential_ast<nset_fhq<int, nless<int>, true, sum_augment>>(0x0502U) == 0));
    ntest((differential_ast<nset_splay<int, nless<int>, true, sum_augment>>(0x0503U) == 0));

    nset_fhq<char, nless<char>, true, string_augment> fhq;
    nset_splay<char, nless<char>, true, string_augment> splay;
    for (char value : {'d', 'a', 'c', 'b', 'b', 'f', 'e'}) {
        fhq.ins(value);
        splay.ins(value);
    }
    ntest(fhq.root().info() == "abbcdef");
    ntest(splay.root().info() == "abbcdef");

    auto stable = fhq.root();
    ntest(fhq.has('a') && stable.current());
    auto expires_on_rotation = splay.root();
    ntest(splay.has('a') && !expires_on_rotation.current());
}
