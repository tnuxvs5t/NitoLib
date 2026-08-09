#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    vector<string> keys{"delta", "alpha", "gamma", "beta"};
    vector<int> values{40, 10, 30, 20};
    unordered_map<string, int> locate{{"alpha", 1}, {"beta", 3}, {"gamma", 2}, {"delta", 0}};
    auto function = nfunc_bind(nall(keys), nall(values),
                               [&](const string& key) { return locate.at(key); });

    auto selected = nselect(function, vector<int>{3, 1, 3, 0});
    CHECK(selected.len() == 4);
    CHECK(selected.key(0) == "beta" && selected[0] == 20);
    CHECK(selected.key(1) == "alpha" && selected("gamma") == 30);
    selected[2] = 21;
    CHECK(values[3] == 21 && selected[0] == 21);

    auto large = nfilter(function, [](int value) { return value >= 30; });
    CHECK(large.len() == 2 && large.key(0) == "delta" && large.key(1) == "gamma");
    auto ordered = norder(function);
    CHECK((ncollect(ordered) == vector<int>{10, 21, 30, 40}));
    CHECK((values == vector<int>{40, 10, 30, 21}));
    for (int i = 0; i < ordered.len(); ++i) CHECK(ordered[i] == function(ordered.key(i)));
    auto reversed_domain = nselect(function, nreverse(nrange(function.len())));
    CHECK(reversed_domain.key(0) == "beta" && reversed_domain.key(3) == "delta");
    CHECK(reversed_domain[1] == function("gamma"));

    vector<char> letters{'x', 'y', 'z', 'w'};
    auto zipped = ncollect(nzip(nall(values), nall(letters)));
    static_assert(same_as<decltype(zipped), vector<tuple<int, char>>>);
    CHECK(zipped[2] == tuple(30, 'z'));
    values[2] = 99;
    CHECK(get<0>(zipped[2]) == 30);

    auto pairs = ncollect(nproduct(nall(letters), nrange(2)));
    static_assert(same_as<decltype(pairs), vector<pair<char, int>>>);
    CHECK(pairs.size() == 8 && pairs[6] == pair('w', 0));

    auto indexed = nindexed(nall(values));
    static_assert(same_as<decltype(get<1>(indexed[0])), int&>);
    CHECK(get<0>(indexed[2]) == 2 && get<1>(indexed[2]) == 99);
    get<1>(indexed[1]) = 13;
    CHECK(values[1] == 13);

    auto indexed_entries = nindexed(nentries(function));
    auto [indexed_position, entry] = indexed_entries[3];
    CHECK(indexed_position == 3 && entry.first == "beta" && entry.second == 21);

    auto selected_move_only = nselect(
        ntabulate(5, [base = make_unique<int>(7)](int i) { return *base + i; }),
        vector<int>{4, 0, 2});
    CHECK((ncollect(move(selected_move_only)) == vector<int>{11, 7, 9}));
    auto move_function = nfunc{
        nrange(4), [base = make_unique<int>(12)](int key) { return *base + key; }};
    auto move_function_pick = nselect(move(move_function), vector<int>{3, 0});
    CHECK(move_function_pick.key(0) == 3 && move_function_pick[1] == 12);

    mt19937 rng(0xD15C0);
    for (int round = 0; round < 12000; ++round) {
        int n = int(rng() % 40);
        vector<int> payload(n), domain(n), inverse(n);
        for (int& value : payload) value = int(rng() % 101) - 50;
        iota(domain.begin(), domain.end(), 0);
        shuffle(domain.begin(), domain.end(), rng);
        for (int i = 0; i < n; ++i) inverse[domain[i]] = i;
        auto f = nfunc_bind(nall(domain), nall(payload),
                            [&](int key) { return inverse[key]; });

        int q = n ? int(rng() % 60) : 0;
        vector<int> positions(q);
        for (int& position : positions) position = int(rng() % n);
        auto view_pick = nselect(nall(payload), positions);
        auto func_pick = nselect(f, move(positions));
        CHECK(view_pick.len() == q && func_pick.len() == q);
        for (int i = 0; i < q; ++i) {
            CHECK(view_pick[i] == func_pick[i]);
            CHECK(func_pick[i] == f(func_pick.key(i)));
        }

        int modulus = 1 + int(rng() % 9), residue = int(rng() % modulus);
        auto filtered = nfilter(f, [=](int value) {
            int normalized = (value % modulus + modulus) % modulus;
            return normalized == residue;
        });
        vector<int> expected_positions;
        for (int i = 0; i < n; ++i) {
            int normalized = (payload[i] % modulus + modulus) % modulus;
            if (normalized == residue) expected_positions.push_back(i);
        }
        CHECK(filtered.len() == int(expected_positions.size()));
        for (int i = 0; i < filtered.len(); ++i) {
            int position = expected_positions[i];
            CHECK(filtered.key(i) == domain[position]);
            CHECK(filtered[i] == payload[position]);
        }

        auto order = norder(f, greater<>{});
        CHECK(order.len() == n);
        for (int i = 1; i < n; ++i) CHECK(order[i - 1] >= order[i]);
        for (int i = 0; i < n; ++i) CHECK(order[i] == f(order.key(i)));
    }
}
