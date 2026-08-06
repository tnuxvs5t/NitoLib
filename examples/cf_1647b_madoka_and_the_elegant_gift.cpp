#include "Nitori.h"

int main() {
    int tests;
    nin >> tests;
    while (tests--) {
        int n, m;
        nin >> n >> m;
        nvector<string> grid(n);
        nrep(row, n)
            nin >> grid[row];

        auto squares = nfunc_value(
            nproduct(nrange(n - 1), nrange(m - 1)),
            [&](auto cell) {
                auto [row, column] = cell;
                return grid[row][column] + grid[row + 1][column] +
                           grid[row][column + 1] + grid[row + 1][column + 1] -
                       4 * '0';
            });

        bool elegant = nnone_of(squares, [](int black) { return black == 3; });
        nprintln(elegant ? "YES" : "NO");
    }
}
