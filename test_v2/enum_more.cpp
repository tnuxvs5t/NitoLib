#include "common.hpp"

int main() {
    nvector<int> a{1, 2, 3}, b{10, 20};
    auto zip = nzip(a, b);
    ntest(zip.len() == 2);
    nfor(item, zip) {
        auto&& [x, y] = item;
        x += y;
    }
    ntest(a == nvector<int>({11, 22, 3}));

    auto product = nproduct(a, b);
    nvector<int> sums;
    nfor(item, product) {
        auto&& [x, y] = item;
        sums.push(x + y);
    }
    ntest(sums == nvector<int>({21, 31, 32, 42, 13, 23}));

    nvector<int> sequence{1, 2, 3, 4, 5};
    auto windows = nwindows(sequence, 3, 2);
    ntest(windows.len() == 2);
    nfor(window, windows) nreverse_inplace(window);
    ntest(sequence == nvector<int>({3, 2, 5, 4, 1}));
    ntest(nwindows(sequence, 9).empty());
    ntest(nwindows(sequence, 0).len() == 6);
}
