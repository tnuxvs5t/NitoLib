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
    nvector<int> repeated{4, 2, 4, 1, 2, 3};
    ntest(nsort_unique(repeated) == 4 && repeated == nvector<int>({1, 2, 3, 4}));
    ntest(nfind_sorted(repeated, 3) == 2 && nfind_sorted(repeated, 8, nless<>{}, 77) == 77);

    struct record {
        int key, payload;
    };
    auto key = [](const record& value) { return value.key; };
    nvector<record> records{{3, 30}, {1, 10}, {2, 20}, {2, 21}};
    nsort(records, nless<>{}, key);
    ntest(records[0].key == 1 && records[0].payload == 10);
    ntest(records[1].key == 2 && records[2].key == 2 && records[3].payload == 30);
    ntest(nlower(records, 2, nless<>{}, key) == 1);
    ntest(nupper(records, 2, nless<>{}, key) == 3);
    ntest(nfind(records, 3, key) == 3);
    ntest(nfind_sorted(records, 2, nless<>{}, key) == 1);
    ntest(nfind_sorted(records, 7, nless<>{}, key, 99) == 99);
    ntest(nfold(records, nadd<int>{}, key) == 8);

    nvector<record> duplicate_records{{2, 20}, {1, 10}, {2, 21}, {1, 11}};
    ntest(nsort_unique(duplicate_records, nless<>{}, nequal<>{}, key) == 2);
    ntest(duplicate_records[0].key == 1 && duplicate_records[1].key == 2);

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
