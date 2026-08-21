#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    vector<string> keys{"delta", "alpha", "gamma", "beta"};
    vector<nidx_t> values{40, 10, 30, 20};
    unordered_map<string, nidx_t> locate{{"alpha", 1}, {"beta", 3}, {"gamma", 2}, {"delta", 0}};
    auto function = nanchors(nall(keys), nall(values),
                               [&](const string& key) { return locate.at(key); });

    auto selected = nselect(function, vector<nidx_t>{3, 1, 3, 0});
    CHECK(selected.len() == 4);
    CHECK(selected.key(0) == "beta" && selected[0] == 20);
    CHECK(selected.key(1) == "alpha" && selected("gamma") == 30);
    selected[2] = 21;
    CHECK(values[3] == 21 && selected[0] == 21);

    auto large = nfilter(function, [](nidx_t value) { return value >= 30; });
    CHECK(large.len() == 2 && large.key(0) == "delta" && large.key(1) == "gamma");
    auto ordered = norder(function);
    CHECK((ncollect(ordered) == vector<nidx_t>{10, 21, 30, 40}));
    CHECK((values == vector<nidx_t>{40, 10, 30, 21}));
    for (nidx_t i = 0; i < ordered.len(); ++i) CHECK(ordered[i] == function(ordered.key(i)));
    auto reversed_domain = nselect(function, nreverse(nrange(function.len())));
    CHECK(reversed_domain.key(0) == "beta" && reversed_domain.key(3) == "delta");
    CHECK(reversed_domain[1] == function("gamma"));

    vector<char> letters{'x', 'y', 'z', 'w'};
    auto zipped = ncollect(nzip(nall(values), nall(letters)));
    static_assert(same_as<decltype(zipped), vector<tuple<nidx_t, char>>>);
    CHECK(zipped[2] == tuple(30, 'z'));
    values[2] = 99;
    CHECK(get<0>(zipped[2]) == 30);

    auto pairs = ncollect(nproduct(nall(letters), nrange(2)));
    static_assert(same_as<decltype(pairs), vector<pair<char, nidx_t>>>);
    CHECK(pairs.size() == 8 && pairs[6] == pair('w', 0));

    auto indexed = nindexed(nall(values));
    static_assert(same_as<decltype(get<1>(indexed[0])), nidx_t&>);
    CHECK(get<0>(indexed[2]) == 2 && get<1>(indexed[2]) == 99);
    get<1>(indexed[1]) = 13;
    CHECK(values[1] == 13);

    auto indexed_entries = nindexed(nentries(function));
    auto [indexed_position, entry] = indexed_entries[3];
    CHECK(indexed_position == 3 && entry.first == "beta" && entry.second == 21);

    auto selected_move_only = nselect(
        ntabulate(5, [base = make_unique<nidx_t>(7)](nidx_t i) { return *base + i; }),
        vector<nidx_t>{4, 0, 2});
    CHECK((ncollect(move(selected_move_only)) == vector<nidx_t>{11, 7, 9}));
    auto move_function = nfunc{
        nrange(4), [base = make_unique<nidx_t>(12)](nidx_t key) { return *base + key; }};
    auto move_function_pick = nselect(move(move_function), vector<nidx_t>{3, 0});
    CHECK(move_function_pick.key(0) == 3 && move_function_pick[1] == 12);

    mt19937 rng(0xD15C0);
    for (nidx_t round = 0; round < 12000; ++round) {
        nidx_t n = nidx_t(rng() % 40);
        vector<nidx_t> payload(n), domain(n), inverse(n);
        for (nidx_t& value : payload) value = nidx_t(rng() % 101) - 50;
        iota(domain.begin(), domain.end(), 0);
        shuffle(domain.begin(), domain.end(), rng);
        for (nidx_t i = 0; i < n; ++i) inverse[domain[i]] = i;
        auto f = nanchors(nall(domain), nall(payload),
                            [&](nidx_t key) { return inverse[key]; });

        nidx_t q = n ? nidx_t(rng() % 60) : 0;
        vector<nidx_t> positions(q);
        for (nidx_t& position : positions) position = nidx_t(rng() % n);
        auto view_pick = nselect(nall(payload), positions);
        auto func_pick = nselect(f, move(positions));
        CHECK(view_pick.len() == q && func_pick.len() == q);
        for (nidx_t i = 0; i < q; ++i) {
            CHECK(view_pick[i] == func_pick[i]);
            CHECK(func_pick[i] == f(func_pick.key(i)));
        }

        nidx_t modulus = 1 + nidx_t(rng() % 9), residue = nidx_t(rng() % modulus);
        auto filtered = nfilter(f, [=](nidx_t value) {
            nidx_t normalized = (value % modulus + modulus) % modulus;
            return normalized == residue;
        });
        vector<nidx_t> expected_positions;
        for (nidx_t i = 0; i < n; ++i) {
            nidx_t normalized = (payload[i] % modulus + modulus) % modulus;
            if (normalized == residue) expected_positions.push_back(i);
        }
        CHECK(filtered.len() == nidx_t(expected_positions.size()));
        for (nidx_t i = 0; i < filtered.len(); ++i) {
            nidx_t position = expected_positions[i];
            CHECK(filtered.key(i) == domain[position]);
            CHECK(filtered[i] == payload[position]);
        }

        auto order = norder(f, greater<>{});
        CHECK(order.len() == n);
        for (nidx_t i = 1; i < n; ++i) CHECK(order[i - 1] >= order[i]);
        for (nidx_t i = 0; i < n; ++i) CHECK(order[i] == f(order.key(i)));
    }
}
