#include "../src-v4/func.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

template <class T>
concept can_nall = requires(T&& x) { nall(forward<T>(x)); };

using borrowed = decltype(nall(declval<vector<nidx_t>&>()));
static_assert(can_nall<vector<nidx_t>&>);
static_assert(can_nall<const vector<nidx_t>&>);
static_assert(!can_nall<vector<nidx_t>>);
static_assert(ranges::random_access_range<borrowed>);
static_assert(same_as<decltype(declval<const borrowed&>()[0]), nidx_t&>);

int main() {
    vector<nidx_t> a{9, 1, 7, 3, 5};
    const auto all = nall(a);
    all[0] = 4;
    CHECK(a[0] == 4);

    auto middle = nreverse(nsub(nall(a), 1, 5));
    CHECK((vector<nidx_t>(middle.begin(), middle.end()) == vector<nidx_t>{5, 3, 7, 1}));
    middle[1] = 30;
    CHECK(a[3] == 30);

    auto arithmetic = ntabulate(
        6,
        [](nidx_t i) { return 10 + 3 * i; },
        [](nidx_t x) { return (x - 10) / 3; }
    );
    static_assert(requires { arithmetic.inverse(10); });
    for (nidx_t i = 0; i < arithmetic.len(); ++i)
        CHECK(arithmetic.inverse(arithmetic[i]) == i);

    auto projected = nproject(nall(a), [](nidx_t& x) -> nidx_t& { return x; });
    auto mapped = nmap(nall(a), [](nidx_t& x) -> nidx_t& { return x; });
    static_assert(same_as<decltype(projected[0]), nidx_t&>);
    static_assert(same_as<decltype(mapped[0]), nidx_t>);
    CHECK(mapped[0] == 4);

    ranges::sort(all);
    CHECK((a == vector<nidx_t>{1, 4, 5, 7, 30}));

    vector<nidx_t> pick{4, 0, 4, 2};
    auto gathered = ngather(nall(a), nall(pick));
    CHECK(gathered[0] == 30 && gathered[1] == 1);
    gathered[2] = 31;
    CHECK(a[4] == 31);

    auto invertible_gather = ngather(nrange(10, 20), nrange(3, 8));
    for (nidx_t i = 0; i < invertible_gather.len(); ++i)
        CHECK(invertible_gather.inverse(invertible_gather[i]) == i);

    vector<char> letters{'a', 'b', 'c'};
    auto zipped = nzip(nall(a), nall(letters), nrange(10));
    CHECK(zipped.len() == 3);
    get<0>(zipped[1]) = 40;
    get<1>(zipped[2]) = 'z';
    CHECK(a[1] == 40 && letters[2] == 'z' && get<2>(zipped[2]) == 2);

    auto pair_product = nproduct(nrange(10, 14), nrange(-3, 2));
    for (nidx_t i = 0; i < pair_product.len(); ++i) {
        auto key = pair_product[i];
        CHECK(pair_product.inverse(key) == i);
    }

    auto cube = nproduct(nrange(2), nrange(2), nrange(2));
    CHECK((cube[5] == tuple{1, 0, 1}));
    for (nidx_t i = 0; i < cube.len(); ++i)
        CHECK(cube.inverse(cube[i]) == i);

    auto keyed = nfunc{
        nproduct(nrange(2), nrange(3)),
        [](const pair<nidx_t, nidx_t>& key) { return 10 * key.first + key.second; }
    };
    CHECK(keyed[4] == 11 && keyed(1, 2) == 12);

    auto entries = nentries(keyed);
    CHECK(entries[4].first.first == 1 && entries[4].first.second == 1 &&
          entries[4].second == 11);
    auto values = nvalues(keyed);
    CHECK(values[5] == 12);

    auto transformed = nmap_values(move(keyed), [](nidx_t x) { return x + 1; });
    CHECK(transformed(1, 2) == 13);

    vector<pair<nidx_t, nidx_t>> subset{{1, 0}, {0, 2}};
    auto restricted = nredomain(transformed, nall(subset));
    CHECK(restricted[0] == 11 && restricted[1] == 3);

    vector<string> keys{"north", "east", "south", "west"};
    vector<nidx_t> payload{2, 3, 5, 7};
    auto anchored = nanchors(nall(keys), nall(payload));
    static_assert(same_as<decltype(anchored(string("north"))), nidx_t&>);
    CHECK(anchored("west") == 7);
    anchored("south") = 50;
    CHECK(payload[2] == 50);

    auto dense = nanchors(nall(payload));
    CHECK(dense.key(2) == 2 && dense[2] == 50 && dense(3) == 7);

    auto move_only = ntabulate(4, [p = make_unique<nidx_t>(7)](nidx_t i) {
        return *p + i;
    });
    auto backwards = nreverse(move(move_only));
    CHECK(backwards[0] == 10 && backwards[3] == 7);

    auto move_function = nfunc{
        nrange(4), [p = make_unique<nidx_t>(6)](nidx_t i) { return *p + i; }
    };
    auto borrowed_values = nvalues(move_function);
    auto borrowed_entries = nentries(move_function);
    CHECK(borrowed_values[3] == 9 && borrowed_entries[2].second == 8);
    auto owned_entries = nentries(nfunc{
        nrange(2), [p = make_unique<nidx_t>(9)](nidx_t i) { return *p + i; }
    });
    CHECK(owned_entries[0].second == 9 && owned_entries[1].second == 10);

    auto locate = nlocate(pair_product);
    CHECK(locate(pair{11, -1}) == 7);

    mt19937 rng(0xC0FFEE);
    for (int round = 0; round < 4000; ++round) {
        nidx_t n = nidx_t(rng() % 33), m = nidx_t(rng() % 25);
        vector<nidx_t> x(n), y(m);
        for (nidx_t& value : x) value = nidx_t(rng() % 2001) - 1000;
        for (nidx_t& value : y) value = nidx_t(rng() % 2001) - 1000;

        nidx_t left = n ? nidx_t(rng() % (n + 1)) : 0;
        nidx_t right = left + nidx_t(rng() % (n - left + 1));
        auto reversed = nreverse(nsub(nall(x), left, right));
        for (nidx_t i = 0; i < reversed.len(); ++i)
            CHECK(reversed[i] == x[right - 1 - i]);

        auto zipped_random = nzip(nall(x), nall(y));
        CHECK(zipped_random.len() == min(n, m));
        for (nidx_t i = 0; i < zipped_random.len(); ++i)
            CHECK(get<0>(zipped_random[i]) == x[i] && get<1>(zipped_random[i]) == y[i]);

        auto product_random = nproduct(nall(x), nall(y));
        CHECK(product_random.len() == n * m);
        if (n && m) {
            nidx_t i = nidx_t(rng() % product_random.len());
            auto key = product_random[i];
            CHECK(key.first == x[i / m] && key.second == y[i % m]);
        }
    }

    cout << "v4 view/func: projection, inverse, product, key/value binding passed\n";
}
