#include "common.hpp"

template <class A>
concept ncan_view_temporary = requires(A&& value) { nall(forward<A>(value)); };

int main() {
    static_assert(nview_object<decltype(nzip(declval<nvector<int>&>(),
                                            declval<nvector<int>&>()))>);
    static_assert(nview_object<decltype(nproduct(declval<nvector<int>&>(),
                                                declval<nvector<int>&>()))>);
    static_assert(nview_object<decltype(nwindows(declval<nvector<int>&>(), 2))>);
    static_assert(nview_object<nview<int>>);
    static_assert(!ncan_view_temporary<nvector<int>>);

    nvector<int> a{1, 2, 3}, b{10, 20};
    auto zip = nzip(a, b);
    ntest(zip.len() == 2);
    nfor(item, zip) {
        auto&& [x, y] = item;
        x += y;
    }
    ntest(a == nvector<int>({11, 22, 3}));

    auto pairs = ncollect(nzip(a, b));
    static_assert(same_as<decltype(pairs), nvector<pair<int, int>>>);
    a[0] = 100;
    b[0] = 200;
    ntest((pairs[0] == pair<int, int>{11, 10}));
    a[0] = 11;
    b[0] = 10;

    auto generated = ncollect(nrange(2, 10, 2));
    static_assert(same_as<decltype(generated), nvector<int>>);
    ntest((generated == nvector<int>{2, 4, 6, 8}));

    nvector<int> tiles{1, 2, 3};
    auto deep_windows = ncollect(nproject(nwindows(tiles, 2), [](auto window) {
        return ncollect(window);
    }));
    static_assert(same_as<decltype(deep_windows), nvector<nvector<int>>>);
    tiles[1] = 99;
    ntest((deep_windows == nvector<nvector<int>>{{1, 2}, {2, 3}}));

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

    auto reversed_slice = nreverse(nsub(sequence, 1, 5));
    ntest((nvector<int>{reversed_slice[0], reversed_slice[1], reversed_slice[2], reversed_slice[3]} ==
           nvector<int>{1, 4, 5, 2}));

    nvector<int> other{10, 20, 30, 40, 50};
    auto composed_zip = nzip(nsub(sequence, 0, 3), nreverse(nsub(other, 1, 4)));
    ntest(composed_zip.len() == 3);
    nfor(item, composed_zip) {
        auto&& [x, y] = item;
        x += y;
    }
    ntest((sequence == nvector<int>{43, 32, 25, 4, 1}));

    auto detached_window = nwindows(nsub(sequence, 1, 5), 2)[1];
    detached_window[0] = 99;
    ntest(sequence[2] == 99 && detached_window[1] == sequence[3]);

    auto from_local_view = [&] {
        auto local = nsub(sequence, 0, 4);
        return nreverse(local);
    }();
    ntest(from_local_view[0] == sequence[3] && from_local_view[3] == sequence[0]);

    auto composed_product = nproduct(nsub(sequence, 0, 2), nsub(other, 0, 2));
    ntest(composed_product.len() == 4 && composed_product[3].first == sequence[1] &&
          composed_product[3].second == other[1]);

    nvector<int> sortable{9, 4, 7, 1, 8};
    nsort(nreverse(nsub(sortable, 1, 4)));
    ntest((sortable == nvector<int>{9, 7, 4, 1, 8}));
    nreverse_inplace(nsub(sortable, 1, 4));
    ntest((sortable == nvector<int>{9, 1, 4, 7, 8}));
}
