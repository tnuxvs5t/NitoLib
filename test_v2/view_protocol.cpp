#include "common.hpp"

template <class A>
concept can_take_rvalue_owner = requires(A value) { nall(move(value)); };

struct dynamic_range {
    using nrange_tag = void;
    int bias = 0;
    int len() const { return 4; }
    int operator[](int index) const { return bias + index; }
};

struct column_major {
    int storage[6]{0, 1, 2, 3, 4, 5};
    int len() const { return 6; }
    int rows() const { return 2; }
    int cols() const { return 3; }
    int* data() { return storage; }
    const int* data() const { return storage; }
    int& operator[](int index) { return storage[index]; }
    const int& operator[](int index) const { return storage[index]; }
    int& operator()(int row, int column) { return storage[column * rows() + row]; }
    const int& operator()(int row, int column) const {
        return storage[column * rows() + row];
    }
};

int main() {
    static_assert(!can_take_rvalue_owner<nvector<int>>);

    dynamic_range descriptor;
    auto borrowed = nall(descriptor);
    static_assert(nview_object<decltype(borrowed)>);
    descriptor.bias = 10;
    ntest(ncollect(borrowed) == nvector<int>({10, 11, 12, 13}));

    auto owned_descriptor = nall(dynamic_range{20});
    ntest(ncollect(owned_descriptor) == nvector<int>({20, 21, 22, 23}));

    int raw[]{0, 1, 2, 3, 4, 5};
    auto all = nview(raw);
    ntest(ncollect(nstride(all, 0, all.len(), 2)) == nvector<int>({0, 2, 4}));
    ntest(ncollect(nstride(all, all.len() - 1, -1, -2)) ==
          nvector<int>({5, 3, 1}));
    ntest(nstride(all, all.len(), all.len(), -1).empty());

    struct record {
        int key, payload;
    };
    nvector<record> records{{3, 30}, {1, 10}, {2, 20}};
    auto key = &record::key;
    nsort(records, nless<>{}, key);
    ntest(records[0].payload == 10 && records[1].payload == 20 &&
          records[2].payload == 30);
    ntest(nlower(records, 2, nless<>{}, key) == 1);

    auto grid = nview(raw, 2, 3);
    nsort(ncolumn(grid, 1), ngreater<>{});
    ntest(grid(0, 1) == 4 && grid(1, 1) == 1);
    auto diagonal = ndiagonal(grid);
    auto snapshot = ncollect(diagonal);
    diagonal[0] = 99;
    ntest(snapshot[0] != diagonal[0]);

    auto lambda_grid = nview(2, 3, [&](int row, int column) -> int& {
        return raw[row * 3 + column];
    });
    static_assert(nview_object<decltype(lambda_grid)>);
    static_assert(!ncontiguous_indexed<decltype(lambda_grid)>);
    int first = lambda_grid(0, 0), last = lambda_grid(0, 2);
    nreverse_inplace(nrow(lambda_grid, 0));
    ntest(lambda_grid(0, 0) == last && lambda_grid(0, 2) == first);

    column_major matrix;
    auto logical_row = nrow(matrix, 0);
    static_assert(!ncontiguous_indexed<decltype(logical_row)>);
    ntest(ncollect(logical_row) == nvector<int>({0, 2, 4}));
}
