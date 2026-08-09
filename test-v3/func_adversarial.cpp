#include "../src-v3/func.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    vector<string> keys{"gamma", "alpha", "delta", "beta", "alpha"};
    vector<int> values{10, 20, 30, 40};
    unordered_map<string, int> locate{{"alpha", 0}, {"beta", 1}, {"gamma", 2}, {"delta", 3}};

    auto f = nfunc_bind(nall(keys), nall(values),
                        [&](const string& key) { return locate.at(key); });
    CHECK(f.len() == 5 && f.key(0) == "gamma");
    CHECK(f[0] == 30 && f[1] == 10 && f("beta") == 20 && f[4] == 10);

    const auto shallow = f;
    shallow("delta") = 41;
    CHECK(values[3] == 41);

    auto identity_values = nmap_values(f, [](int& x) -> int& { return x; });
    static_assert(same_as<decltype(identity_values("alpha")), int&>);
    identity_values("alpha") = 11;
    CHECK(values[0] == 11);

    auto labels = nmap_values(f, [](int x) { return to_string(x) + "!"; });
    CHECK(labels[2] == "41!" && labels("gamma") == "30!");

    vector<string> subset{"beta", "beta", "alpha"};
    auto restricted = nredomain(f, nall(subset));
    CHECK(restricted.len() == 3 && restricted[0] == 20 && restricted[2] == 11);

    vector<int> positions{4, 0, 1, 1};
    auto selected = nselect_positions(f, nall(positions));
    CHECK(selected.len() == 4);
    CHECK(selected.key(0) == "alpha" && selected[0] == 11);
    CHECK(selected.key(1) == "gamma" && selected[1] == 30);

    auto entries = nentries(f);
    static_assert(same_as<decltype(entries[0].first), string&>);
    static_assert(same_as<decltype(entries[0].second), int&>);
    entries[3].second = 22;
    CHECK(values[1] == 22);

    auto values_view = nvalues(f);
    values_view[2] = 42;
    CHECK(values[3] == 42);

    auto dense = nfunc_bind(nall(values));
    CHECK(dense.key(3) == 3 && dense(3) == 42 && dense[1] == 22);

    auto move_only = nfunc{nrange(5),
                           [p = make_unique<int>(6)](int key) { return *p * key; }};
    auto composed = ncompose([](int x) { return x + 1; }, move(move_only));
    CHECK(composed.len() == 5 && composed[0] == 1 && composed(4) == 25);

    int calls = 0;
    auto stateful = nfunc{nrange(3), [calls](int x) mutable { return x + calls++; }};
    const auto stateful_const = move(stateful);
    CHECK(stateful_const[0] == 0);
    CHECK(stateful_const[0] == 1);
}
