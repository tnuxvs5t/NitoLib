#include "common.hpp"

struct record {
    int key, payload;
};

struct squares {
    int count;

    struct cursor {
        int index = 0, count = 0;
        bool ok() const { return index < count; }
        int val() const { return index * index; }
        int idx() const { return index; }
        void next() { ++index; }
    };

    cursor enumerate() const { return {0, count}; }
};

int main() {
    nvector<int> source{1, 2, 3, 4};
    nvector<int> destination(4, -1);
    nassign(destination, source);
    ntest(destination == source);

    nfill(nsub(destination, 1, 3), 9);
    ntest(destination == nvector<int>({1, 9, 9, 4}));

    nmatrix<int> matrix(2, 4, 0);
    nassign(matrix.row(1), source);
    ntest(matrix(1, 0) == 1 && matrix(1, 3) == 4);

    nvector<record> records{{3, 30}, {1, 10}, {4, 40}, {1, 11}};
    nvector<int> keys(4);
    nassign(keys, records, &record::key);
    ntest(keys == nvector<int>({3, 1, 4, 1}));

    nassign(keys, squares{4}, [](int square) { return square + 1; });
    ntest(keys == nvector<int>({1, 2, 5, 10}));
    nassign(keys, records, &record::key);

    ndeque<int> left, right;
    for (int x : {1, 2, 3})
        left.pushr(x);
    for (int x : {7, 8, 9})
        right.pushr(x);
    nswap_ranges(left, right);
    ntest(left[0] == 7 && left[2] == 9 && right[0] == 1 && right[2] == 3);

    ntest(nfind_if(records, [](int key) { return key % 2 == 0; }, &record::key) == 2);
    ntest(nfind_if(records, [](int key) { return key == 8; }, &record::key, 77) == 77);
    ntest(ncontains(records, 4, &record::key));
    ntest(!ncontains(records, 8, &record::key));
    ntest(ncount(records, 1, &record::key) == 2);
    ntest(ncount_if(records, [](int key) { return key & 1; }, &record::key) == 3);

    ntest(nall_of(nrange(1, 5), [](int x) { return x > 0; }));
    ntest(nany_of(records, [](int value) { return value == 40; }, &record::payload));
    ntest(nnone_of(records, [](int key) { return key < 0; }, &record::key));
    ntest(nall_of(nrange(0), [](int) { return false; }));
    ntest(!nany_of(nrange(0), [](int) { return true; }));
    ntest(nnone_of(nrange(0), [](int) { return true; }));

    ntest(nsame(keys, nproject(records, &record::key)));
    ntest(nsame(records, keys, nequal<>{}, &record::key, nidentity{}));
    ntest(!nsame(nrange(3), nrange(4)));

    ntest(nargmin(records, nless<>{}, &record::key) == 1);
    ntest(nargmax(records, nless<>{}, &record::key) == 2);
    ntest(nargmin(nvector<int>{}) == npos && nargmax(nvector<int>{}) == npos);
}
