#include "../src-v3/func.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

template <class T>
concept can_nall = requires(T&& x) { nall(forward<T>(x)); };

using borrowed_vector = decltype(nall(declval<vector<nidx_t>&>()));
static_assert(can_nall<vector<nidx_t>&>);
static_assert(can_nall<const vector<nidx_t>&>);
static_assert(!can_nall<vector<nidx_t>>);
static_assert(ranges::random_access_range<borrowed_vector>);
static_assert(same_as<decltype(declval<const borrowed_vector&>()[0]), nidx_t&>);

int main() {
    vector<nidx_t> a{9, 1, 7, 3, 5};
    const auto all = nall(a);
    all[0] = 4;
    CHECK(a[0] == 4 && all.len() == 5);

    auto middle = nreverse(nsub(nall(a), 1, 5));
    vector<nidx_t> expected{5, 3, 7, 1};
    CHECK(equal(middle.begin(), middle.end(), expected.begin(), expected.end()));
    middle[1] = 30;
    CHECK(a[3] == 30);

    auto projected = nproject(nall(a), [](nidx_t& x) -> nidx_t& { return x; });
    auto mapped = nmap(nall(a), [](nidx_t& x) -> nidx_t& { return x; });
    static_assert(same_as<decltype(projected[0]), nidx_t&>);
    static_assert(same_as<decltype(mapped[0]), nidx_t>);

    ranges::sort(all);
    CHECK((a == vector<nidx_t>{1, 4, 5, 7, 30}));

    vector<nidx_t> pick{4, 0, 4, 2};
    auto gathered = ngather(nall(a), nall(pick));
    CHECK(gathered.len() == 4 && gathered[0] == 30 && gathered[1] == 1);
    gathered[2] = 31;
    CHECK(a[4] == 31);

    vector<char> letters{'a', 'b', 'c'};
    auto zipped = nzip(nall(a), nall(letters), nrange(10));
    CHECK(zipped.len() == 3);
    get<0>(zipped[1]) = 40;
    get<1>(zipped[2]) = 'z';
    CHECK(a[1] == 40 && letters[2] == 'z' && get<2>(zipped[2]) == 2);

    auto product = nproduct(nall(letters), nrange(2));
    CHECK(product.len() == 6);
    vector<pair<char, nidx_t>> wanted{{'a', 0}, {'a', 1}, {'b', 0},
                                   {'b', 1}, {'z', 0}, {'z', 1}};
    for (nidx_t i = 0; i < product.len(); ++i)
        CHECK(product[i].first == wanted[i].first && product[i].second == wanted[i].second);
    product[2].first = 'B';
    CHECK(letters[1] == 'B');
    CHECK(nproduct(nall(a), nrange(0)).empty());

    auto keyed = nfunc{
        nproduct(nrange(2), nrange(3)),
        [](const pair<nidx_t, nidx_t>& key) { return 10 * key.first + key.second; }
    };
    CHECK(keyed[4] == 11);
    CHECK((keyed(1, 2) == keyed(pair{1, 2})));
    CHECK(keyed(1, 2) == 12);

    auto cube = nfunc{
        nproduct(nrange(2), nrange(2), nrange(2)),
        [](const tuple<nidx_t, nidx_t, nidx_t>& key) {
            return 4 * get<0>(key) + 2 * get<1>(key) + get<2>(key);
        }
    };
    CHECK(cube[5] == 5);
    CHECK((cube(1, 0, 1) == cube(tuple{1, 0, 1})));
    CHECK(cube(1, 0, 1) == 5);

    auto transformed = nmap_values(move(keyed), [](nidx_t value) { return value + 1; });
    CHECK(transformed(1, 2) == 13);

    auto bound = nanchors(
        nproduct(nrange(2), nrange(3)), nrange(6),
        [](const pair<nidx_t, nidx_t>& key) { return 3 * key.first + key.second; }
    );
    CHECK(bound[5] == 5 && (bound(pair{1, 2}) == 5));

    auto move_only = ntabulate(4, [p = make_unique<nidx_t>(7)](nidx_t i) { return *p + i; });
    auto backwards = nreverse(move(move_only));
    CHECK(backwards[0] == 10 && backwards[3] == 7);
}
