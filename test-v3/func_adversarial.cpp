#include "../src-v3/func.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    vector<string> keys{"gamma", "alpha", "delta", "beta", "alpha"};
    vector<nidx_t> values{10, 20, 30, 40};
    unordered_map<string, nidx_t> locate{{"alpha", 0}, {"beta", 1}, {"gamma", 2}, {"delta", 3}};

    auto f = nanchors(nall(keys), nall(values),
                        [&](const string& key) { return locate.at(key); });
    static_assert(same_as<decltype(nkeys(f)), decltype(f.domain)&>);
    static_assert(same_as<decltype(nkeys(as_const(f))), const decltype(f.domain)&>);
    static_assert(same_as<decltype(nkeys(move(as_const(f)))), decltype(f.domain)>);
    CHECK(f.len() == 5 && f.key(0) == "gamma");
    CHECK(f[0] == 30 && f[1] == 10 && f("beta") == 20 && f[4] == 10);

    const auto shallow = f;
    shallow("delta") = 41;
    CHECK(values[3] == 41);

    auto identity_values = nmap_values(f, [](nidx_t& x) -> nidx_t& { return x; });
    static_assert(same_as<decltype(identity_values("alpha")), nidx_t&>);
    identity_values("alpha") = 11;
    CHECK(values[0] == 11);

    auto labels = nmap_values(f, [](nidx_t x) { return to_string(x) + "!"; });
    CHECK(labels[2] == "41!" && labels("gamma") == "30!");

    vector<string> subset{"beta", "beta", "alpha"};
    auto restricted = nredomain(f, nall(subset));
    CHECK(restricted.len() == 3 && restricted[0] == 20 && restricted[2] == 11);

    vector<nidx_t> positions{4, 0, 1, 1};
    auto selected = nselect_positions(f, nall(positions));
    CHECK(selected.len() == 4);
    CHECK(selected.key(0) == "alpha" && selected[0] == 11);
    CHECK(selected.key(1) == "gamma" && selected[1] == 30);

    auto entries = nentries(f);
    static_assert(same_as<decltype(entries[0].first), string&>);
    static_assert(same_as<decltype(entries[0].second), nidx_t&>);
    entries[3].second = 22;
    CHECK(values[1] == 22);

    auto values_view = nvalues(f);
    values_view[2] = 42;
    CHECK(values[3] == 42);

    auto dense = nanchors(nall(values));
    CHECK(dense.key(3) == 3 && dense(3) == 42 && dense[1] == 22);

    vector<string> anchor_keys{"north", "east", "south", "west"};
    vector<nidx_t> anchor_values{2, 3, 5, 7};
    auto anchored = nanchors(nall(anchor_keys), nall(anchor_values));
    static_assert(same_as<decltype(anchored(string("north"))), nidx_t&>);
    CHECK(anchored.len() == 4 && anchored.key(2) == "south");
    CHECK(anchored[1] == 3 && anchored("west") == 7);
    anchored("south") = 50;
    CHECK(anchor_values[2] == 50 && nvalues(anchored)[2] == 50);
    vector<string> reordered_keys{"west", "north", "west"};
    auto reordered = nredomain(anchored, nall(reordered_keys));
    CHECK(reordered[0] == 7 && reordered[1] == 2 && reordered[2] == 7);

    vector<string> no_keys;
    vector<nidx_t> no_values;
    auto empty_binding = nanchors(nall(no_keys), nall(no_values));
    CHECK(empty_binding.len() == 0);

    auto move_only = nfunc{nrange(5),
                           [p = make_unique<nidx_t>(6)](nidx_t key) { return *p * key; }};
    auto borrowed_move_values = nvalues(move_only);
    auto borrowed_move_entries = nentries(move_only);
    CHECK(borrowed_move_values[3] == 18);
    CHECK(borrowed_move_entries[4].first == 4 && borrowed_move_entries[4].second == 24);
    CHECK(move_only(2) == 12);
    auto composed = ncompose([](nidx_t x) { return x + 1; }, move(move_only));
    CHECK(composed.len() == 5 && composed[0] == 1 && composed(4) == 25);

    auto borrowed_state = nfunc{nrange(1), [calls = 0](nidx_t) mutable { return calls++; }};
    auto borrowed_state_values = nvalues(borrowed_state);
    CHECK(borrowed_state_values[0] == 0);
    CHECK(borrowed_state[0] == 1);

    auto owned_entries = nentries(nfunc{
        nrange(2), [p = make_unique<nidx_t>(9)](nidx_t key) { return *p + key; }});
    CHECK(owned_entries[0].second == 9 && owned_entries[1].second == 10);

    nidx_t calls = 0;
    auto stateful = nfunc{nrange(3), [calls](nidx_t x) mutable { return x + calls++; }};
    const auto stateful_const = move(stateful);
    CHECK(stateful_const[0] == 0);
    CHECK(stateful_const[0] == 1);
}
