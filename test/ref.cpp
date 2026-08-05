#include "common.hpp"

template <class A>
concept ncan_borrow_rvalue = requires(A&& a) { nall(move(a)); };

int main() {
    static_assert(nindexed<nview<int>>);
    static_assert(ncontiguous_indexed<nview<int>>);
    static_assert(nswappable_indexed<nview<int>>);
    static_assert(!nswappable_indexed<nview<const int>>);
    static_assert(!ncan_borrow_rvalue<nvector<int>>);

    int raw[]{9, 1, 8, 2, 7, 3};
    nview<int> all(raw);
    auto deduced = nview(raw, 6);
    static_assert(same_as<decltype(deduced), nview<int>>);
    ntest(all.len() == 6 && all[1] == 1 && all.get(9) == nullptr);

    auto middle = nsub(all, 1, 5);
    static_assert(ncontiguous_indexed<decltype(middle)>);
    ntest(middle.data() == raw + 1);
    nreverse_inplace(middle);
    ntest((vector<int>(raw, raw + 6) == vector<int>{9, 7, 2, 8, 1, 3}));

    auto odd = nstride(all, 1, 6, 2);
    nsort(odd);
    ntest((vector<int>(raw, raw + 6) == vector<int>{9, 3, 2, 7, 1, 8}));

    auto lambda = nview(3, [&](int i) -> int& { return raw[2 * i]; });
    static_assert(!ncontiguous_indexed<decltype(lambda)>);
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
    nview<const int> read_only(frozen);
    nview<const int> converted(all);
    ntest(converted.data() == raw && converted[0] == all[0]);
    ntest(nfold(read_only) == 6);

    nvector<int> owner{5, 4, 3, 2, 1};
    auto owner_all = nall(owner);
    auto owner_middle = nsub(owner, 1, 4);
    static_assert(ncontiguous_indexed<decltype(owner_all)>);
    static_assert(ncontiguous_indexed<decltype(owner_middle)>);
    nsort(owner_middle);
    ntest(owner == nvector<int>({5, 2, 3, 4, 1}));

    int grid[]{9, 2, 7, 4, 6, 8, 3, 5, 1, 0, 11, 10};
    auto matrix = nview(grid, 3, 4);
    static_assert(nview_object<decltype(matrix)>);
    static_assert(ncontiguous_indexed<decltype(matrix)>);
    ntest(matrix.rows() == 3 && matrix.cols() == 4 && matrix.dim(2) == npos);
    ntest(matrix(2, 3) == 10);
    auto matrix_row = nrow(matrix, 1);
    static_assert(ncontiguous_indexed<decltype(matrix_row)>);
    nsort(matrix_row);
    ntest(matrix(1, 0) == 3 && matrix(1, 3) == 8);
    nsort(ncolumn(matrix, 1));
    ntest(matrix(0, 1) == 0 && matrix(2, 1) == 5);
    nsort(ndiagonal(matrix));
    ntest(matrix(0, 0) <= matrix(1, 1) && matrix(1, 1) <= matrix(2, 2));

    auto transposed_layout = nview(grid, 4, 3, ptrdiff_t{1}, ptrdiff_t{4});
    static_assert(!ncontiguous_indexed<decltype(transposed_layout)>);
    ntest(transposed_layout(2, 1) == matrix(1, 2));
}
