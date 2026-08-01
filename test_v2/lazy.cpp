#include "common.hpp"

struct naffine {
    long long multiply, add;
};

struct nmod_sum {
    static constexpr nlaw laws = nlaw::associative | nlaw::identity | nlaw::commutative;
    long long mod;
    long long id() const { return 0; }
    long long operator()(long long a, long long b) const { return (a + b) % mod; }
};

struct naffine_sum_action {
    long long mod;
    naffine tag_id() const { return {1, 0}; }
    naffine compose(const naffine& newer, const naffine& older) const {
        return {newer.multiply * older.multiply % mod,
                (newer.multiply * older.add + newer.add) % mod};
    }
    long long apply(long long sum, const naffine& tag, int length) const {
        return (tag.multiply * sum + tag.add * length) % mod;
    }
};

template <>
inline constexpr bool naction_laws<naffine_sum_action, long long, naffine> = true;

int main() {
    nvector<long long> values{1, 2, 3, 4, 5};
    nlazy_addsum<long long> addsum(values);
    addsum.apply(1, 4, 10);
    ntest(addsum.fold(0, 5) == 45 && addsum.fold(1, 4) == 39);
    addsum.set(2, -3);
    ntest(addsum.get(2) == -3 && addsum.fold() == 29);

    constexpr long long mod = 1000000007;
    nlazyseg<long long, naffine, nmod_sum, naffine_sum_action> affine(
        values, nmod_sum{mod}, naffine_sum_action{mod});
    affine.apply(0, 5, {2, 1});
    affine.apply(1, 4, {3, 4});
    nvector<long long> expected{3, 19, 25, 31, 11};
    for (int i = 0; i < expected.len(); ++i)
        ntest(affine.get(i) == expected[i]);
    ntest(affine.fold() == nfold(expected));
}
