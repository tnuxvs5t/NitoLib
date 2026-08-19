#include "../src-v3/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

template <class V, class K>
concept inverse_for = requires(V& view, K&& key) {
    view.inverse(forward<K>(key));
};

int main() {
    auto plain = ntabulate(5, [](nidx_t position) { return 2 * position + 1; });
    static_assert(!inverse_for<decltype(plain), nidx_t>);

    auto arithmetic = ntabulate(
        6,
        [](nidx_t position) { return 10 + 3 * position; },
        [](nidx_t key) { return (key - 10) / 3; }
    );
    static_assert(inverse_for<decltype(arithmetic), nidx_t>);
    for (nidx_t i = 0; i < arithmetic.len(); ++i)
        CHECK(arithmetic.inverse(arithmetic[i]) == i);

    auto range = nrange(-7, 12);
    auto middle = nsub(range, 3, 15);
    auto reversed = nreverse(middle);
    auto gathered = ngather(reversed, nrange(2, 9));
    for (nidx_t i = 0; i < gathered.len(); ++i) {
        nidx_t key = gathered[i];
        CHECK(gathered.inverse(key) == i);
        CHECK(gathered[gathered.inverse(key)] == key);
    }

    auto pair_product = nproduct(nrange(10, 14), nrange(-3, 2));
    for (nidx_t i = 0; i < pair_product.len(); ++i) {
        auto key = pair_product[i];
        CHECK(pair_product.inverse(key) == i);
    }

    auto tuple_product = nproduct(
        nrange(5, 8), nrange(-2, 2), nrange(20, 23), nrange(2)
    );
    for (nidx_t i = 0; i < tuple_product.len(); ++i) {
        auto key = tuple_product[i];
        CHECK(tuple_product.inverse(key) == i);
    }

    vector<nidx_t> owner{4, 7, 9};
    auto borrowed = nall(owner);
    auto mapped = nmap(nrange(3), [](nidx_t value) { return value * value; });
    static_assert(!inverse_for<decltype(borrowed), nidx_t>);
    static_assert(!inverse_for<decltype(mapped), nidx_t>);

    mt19937 rng(0x1A2B3C4D);
    for (nidx_t round = 0; round < 20000; ++round) {
        nidx_t first = nidx_t(rng() % 101) - 50;
        nidx_t length = nidx_t(rng() % 40);
        auto source = nrange(first, first + length);
        nidx_t left = length ? nidx_t(rng() % (length + 1)) : 0;
        nidx_t right = left + nidx_t(rng() % (length - left + 1));
        auto view = nreverse(nsub(source, left, right));
        for (nidx_t i = 0; i < view.len(); ++i)
            CHECK(view.inverse(view[i]) == i);

        nidx_t rows = 1 + nidx_t(rng() % 8), columns = 1 + nidx_t(rng() % 8);
        auto cells = nproduct(nrange(first, first + rows), nrange(-9, -9 + columns));
        for (nidx_t i = 0; i < cells.len(); ++i)
            CHECK(cells.inverse(cells[i]) == i);
    }
}
