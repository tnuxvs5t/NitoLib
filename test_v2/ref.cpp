#include "common.hpp"

template <class A>
concept ncan_borrow_rvalue = requires(A&& a) { nall(move(a)); };

int main() {
    static_assert(nindexed<nspan<int>>);
    static_assert(ncontiguous_indexed<nspan<int>>);
    static_assert(nswappable_indexed<nspan<int>>);
    static_assert(!nswappable_indexed<nspan<const int>>);
    static_assert(!ncan_borrow_rvalue<nvector<int>>);

    int raw[]{9, 1, 8, 2, 7, 3};
    nspan<int> all(raw);
    ntest(all.len() == 6 && all[1] == 1 && all.get(9) == nullptr);

    auto middle = all.sub(1, 5);
    nreverse_inplace(middle);
    ntest((vector<int>(raw, raw + 6) == vector<int>{9, 7, 2, 8, 1, 3}));

    auto odd = nstride(all, 1, 3, 2);
    nsort(odd);
    ntest((vector<int>(raw, raw + 6) == vector<int>{9, 3, 2, 7, 1, 8}));

    auto lambda = nview(3, [&](int i) -> int& { return raw[2 * i]; });
    static_assert(sizeof(lambda) <= 2 * sizeof(void*));
    nsort(lambda);
    ntest(raw[0] == 1 && raw[2] == 2 && raw[4] == 9);

    auto alias = lambda;
    alias[0] = 42;
    ntest(raw[0] == 42);
    alias[0] = 1;

    auto snapshot = ncollect(nreverse(lambda));
    static_assert(same_as<decltype(snapshot), nvector<int>>);
    ntest((snapshot == nvector<int>{9, 2, 1}));
    snapshot[0] = -1;
    ntest(raw[4] == 9);

    auto wide = ncollect<long long>(all);
    static_assert(same_as<decltype(wide), nvector<long long>>);
    ntest(wide.len() == all.len() && wide[0] == raw[0]);

    const int frozen[]{3, 1, 2};
    nspan<const int> read_only(frozen);
    ntest(nfold(read_only) == 6);
}
