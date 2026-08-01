#include "common.hpp"

template <class A>
concept nhas_data = requires(A& a) { a.data(); };

int main() {
    static_assert(nhas_data<nvector<int>>);
    static_assert(!nhas_data<ndeque<int>>);
    static_assert(nswappable_indexed<ndeque<int>>);

    nvector<int> a{4, 1, 3, 1, 2};
    nsort(a);
    ntest((a == nvector<int>{1, 1, 2, 3, 4}));
    ntest(nlower(a, 1) == 0 && nupper(a, 1) == 2 && nfind(a, 3) == 3);
    ntest(nfold(a) == 11);
    ntest(nunique(a) == 4 && (a == nvector<int>{1, 2, 3, 4}));

    nvector<string> distinct{"a", "b", "c"};
    ntest(nunique(distinct) == 3 && distinct[0] == "a" && distinct[1] == "b" && distinct[2] == "c");

    ndeque<int> d;
    for (int i = 0; i < 200; ++i)
        (i & 1) ? d.pushl(i) : d.pushr(i);
    nsort(d);
    for (int i = 0; i < d.len(); ++i)
        ntest(d[i] == i);
    nreverse_inplace(d, 50, 150);
    ntest(d[50] == 149 && d[149] == 50);

    narray<int, 2> matrix(array<int, 2>{4, 4});
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            matrix(i, j) = 100 * i + j;
    matrix(0, 0) = 40;
    matrix(1, 1) = 10;
    matrix(2, 2) = 30;
    matrix(3, 3) = 20;

    auto diagonal = nview(4, [&](int i) -> int& { return matrix(i, i); });
    nsort(diagonal);
    ntest(matrix(0, 0) == 10 && matrix(1, 1) == 20 && matrix(2, 2) == 30 && matrix(3, 3) == 40);
    ntest(matrix(0, 1) == 1 && matrix(3, 2) == 302);

    auto row = nview(4, [&](int j) -> int& { return matrix(2, j); });
    nreverse_inplace(row);
    ntest(matrix(2, 0) == 203 && matrix(2, 3) == 200);
}
