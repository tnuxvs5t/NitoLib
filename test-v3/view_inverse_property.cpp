#include "../src-v3/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

template <class V, class K>
concept inverse_for = requires(V& view, K&& key) {
    view.inverse(forward<K>(key));
};

int main() {
    auto plain = ntabulate(5, [](int position) { return 2 * position + 1; });
    static_assert(!inverse_for<decltype(plain), int>);

    auto arithmetic = ntabulate(
        6,
        [](int position) { return 10 + 3 * position; },
        [](int key) { return (key - 10) / 3; }
    );
    static_assert(inverse_for<decltype(arithmetic), int>);
    for (int i = 0; i < arithmetic.len(); ++i)
        CHECK(arithmetic.inverse(arithmetic[i]) == i);

    auto range = nrange(-7, 12);
    auto middle = nsub(range, 3, 15);
    auto reversed = nreverse(middle);
    auto gathered = ngather(reversed, nrange(2, 9));
    for (int i = 0; i < gathered.len(); ++i) {
        int key = gathered[i];
        CHECK(gathered.inverse(key) == i);
        CHECK(gathered[gathered.inverse(key)] == key);
    }

    auto pair_product = nproduct(nrange(10, 14), nrange(-3, 2));
    for (int i = 0; i < pair_product.len(); ++i) {
        auto key = pair_product[i];
        CHECK(pair_product.inverse(key) == i);
    }

    auto tuple_product = nproduct(
        nrange(5, 8), nrange(-2, 2), nrange(20, 23), nrange(2)
    );
    for (int i = 0; i < tuple_product.len(); ++i) {
        auto key = tuple_product[i];
        CHECK(tuple_product.inverse(key) == i);
    }

    vector<int> owner{4, 7, 9};
    auto borrowed = nall(owner);
    auto mapped = nmap(nrange(3), [](int value) { return value * value; });
    static_assert(!inverse_for<decltype(borrowed), int>);
    static_assert(!inverse_for<decltype(mapped), int>);

    mt19937 rng(0x1A2B3C4D);
    for (int round = 0; round < 20000; ++round) {
        int first = int(rng() % 101) - 50;
        int length = int(rng() % 40);
        auto source = nrange(first, first + length);
        int left = length ? int(rng() % (length + 1)) : 0;
        int right = left + int(rng() % (length - left + 1));
        auto view = nreverse(nsub(source, left, right));
        for (int i = 0; i < view.len(); ++i)
            CHECK(view.inverse(view[i]) == i);

        int rows = 1 + int(rng() % 8), columns = 1 + int(rng() % 8);
        auto cells = nproduct(nrange(first, first + rows), nrange(-9, -9 + columns));
        for (int i = 0; i < cells.len(); ++i)
            CHECK(cells.inverse(cells[i]) == i);
    }
}
