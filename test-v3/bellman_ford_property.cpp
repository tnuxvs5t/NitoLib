#include "../src-v3/graph_algo.hpp"
#include "../src-v3/graph_store.hpp"
#include "../src-v3/hash.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct edge { nidx_t to; long long weight; };
constexpr long long inf = 1LL << 60, neg_inf = -inf;
auto cost = [](const edge& item) { return item.weight; };

// Independent all-pairs oracle: Floyd-Warshall and negative diagonal reachability.
void check(const vector<vector<edge>>& adjacency, nidx_t source) {
    nidx_t n = nidx_t(adjacency.size());
    vector<vector<long long>> oracle(n, vector<long long>(n, inf));
    for (nidx_t v = 0; v < n; ++v) oracle[v][v] = 0;
    for (nidx_t from = 0; from < n; ++from)
        for (auto item : adjacency[from])
            oracle[from][item.to] = min(oracle[from][item.to], item.weight);
    for (nidx_t k = 0; k < n; ++k)
        for (nidx_t from = 0; from < n; ++from)
            for (nidx_t to = 0; to < n; ++to)
                if (oracle[from][k] != inf && oracle[k][to] != inf)
                    oracle[from][to] = min(oracle[from][to], oracle[from][k] + oracle[k][to]);
    auto expected = oracle[source];
    bool negative_cycle = false;
    for (nidx_t k = 0; k < n; ++k) if (oracle[source][k] != inf && oracle[k][k] < 0) {
        negative_cycle = true;
        for (nidx_t to = 0; to < n; ++to)
            if (oracle[k][to] != inf) expected[to] = neg_inf;
    }
    auto graph = ngraph{n, [&](nidx_t v) -> const auto& { return adjacency[v]; },
                        [](const edge& item) { return item.to; }};
    auto standard = nbellman_ford(graph, source, cost, inf);
    CHECK(standard.has_value() == !negative_cycle);
    if (standard) CHECK(*standard == expected);
    CHECK(nbellman_ford_closure(graph, source, cost, inf, neg_inf) == expected);

    // Descriptor order is not the key order; ranges may be independent temporaries.
    auto reordered = ngraph{nreverse(nrange(n)), [&](nidx_t v) { return adjacency[v]; },
                            [](const edge& item) { return item.to; }};
    reverse(expected.begin(), expected.end());
    CHECK(nbellman_ford_closure(reordered, source, cost, inf, neg_inf) == expected);
    auto reordered_standard = nbellman_ford(reordered, source, cost, inf);
    CHECK(reordered_standard.has_value() == !negative_cycle);
    if (reordered_standard) CHECK(*reordered_standard == expected);
}

int main() {
    check({{}}, 0);
    check({{{0, 0}}}, 0);
    check({{{0, -1}}}, 0); // V=1 still needs a detection scan.
    check({{}, {{1, -1}, {0, -9}}}, 0); // Unreachable negative cycle cannot seed reachability.
    check({{{1, 7}, {1, -3}}, {{2, 2}}, {}, {}}, 0);
    check({{{1, 3}, {4, 8}}, {{2, -4}}, {{1, 1}, {3, 5}}, {}, {}, {{5, -1}}}, 0);
    check({{{1, -2}}, {{0, 2}, {2, 1}}, {}}, 0); // Zero-weight cycle is finite.
    check({{{1, -1}}, {{2, -1}}, {{0, -1}}}, 0);
    check({{}, {{0, -2}}, {{1, -3}}, {{2, -4}}}, 3); // Reverse chain needs V-1 passes.
    // Closure must propagate even when a downstream label is already extremely small.
    check({{{1, 0}, {3, -10000}}, {{2, -1}}, {{1, 0}, {3, 0}}, {{4, 0}}, {}}, 0);

    mt19937 rng(0xBE11FA);
    for (nidx_t round = 0; round < 2400; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 9);
        vector<vector<edge>> adjacency(n);
        vector<long long> potential(n);
        for (auto& value : potential) value = static_cast<long long>(rng() % 31) - 15;
        for (nidx_t from = 0; from < n; ++from)
            for (nidx_t to = 0; to < n; ++to) if (rng() % 5 == 0) {
                // Alternate arbitrary signed graphs and certified no-negative-cycle graphs.
                long long weight = static_cast<long long>(rng() % 17) - 8;
                if (round % 2) weight = static_cast<long long>(rng() % 8) + potential[to] - potential[from];
                adjacency[from].push_back({to, weight});
                if (rng() % 4 == 0) adjacency[from].push_back({to, weight + 2});
            }
        for (nidx_t source = 0; source < n; ++source) check(adjacency, source);
    }

    vector<string> names{"sink", "source", "middle", "isolated"};
    struct named_edge { string to; long long weight; };
    map<string, vector<named_edge>> adjacency{{"source", {{"middle", -7}, {"sink", 5}}},
                                             {"middle", {{"sink", 2}}}};
    auto named = ngraph{ninvert(nall(names)), [&](const string& v) -> auto& { return adjacency[v]; },
                        [](const named_edge& item) -> const string& { return item.to; }};
    auto named_cost = [](const named_edge& item) { return item.weight; };
    auto answer = nbellman_ford(named, string("source"), named_cost, inf);
    CHECK(answer && (*answer == vector<long long>{-5, 0, -7, inf}));
    CHECK(nbellman_ford_closure(named, string("source"), named_cost, inf, neg_inf) == *answer);

    struct record { nidx_t from, to; long long weight; };
    vector<record> records{{0, 1, -4}, {1, 2, 2}, {0, 2, 9}};
    const auto csr = nmake_csr(4, records, [](const record& item) { return item.from; },
                              [](const record& item) { return item.to; });
    auto csr_answer = nbellman_ford(csr, 0, [](const record& item) { return item.weight; }, inf);
    CHECK(csr_answer && (*csr_answer == vector<long long>{0, -4, -2, inf}));
    CHECK(nbellman_ford_closure(csr, 0, [](const record& item) { return item.weight; }, inf, neg_inf) == *csr_answer);

    // Full-pass early termination, not V unconditional scans; move-only callable accepted.
    vector<vector<edge>> sparse(100);
    sparse[0].push_back({1, -1});
    auto graph = ngraph{100, [&](nidx_t v) -> auto& { return sparse[v]; },
                        [](const edge& item) { return item.to; }};
    nidx_t calls = 0;
    auto counted = [token = make_unique<nidx_t>(0), &calls](const edge& item) {
        ++calls;
        return item.weight + *token;
    };
    auto early = nbellman_ford(graph, 0, move(counted), inf);
    CHECK(early && (*early)[1] == -1 && calls == 3);

    // Only finite operands are added, and callers can deliberately widen W.
    using wide = __int128_t;
    sparse[0][0].weight = numeric_limits<long long>::lowest();
    sparse[1].push_back({2, numeric_limits<long long>::lowest()});
    wide wide_inf = wide(1) << 100;
    auto widened = nbellman_ford(graph, 0, cost, wide_inf);
    CHECK(widened && (*widened)[2] == -(wide(1) << 64));
    CHECK(nbellman_ford_closure(graph, 0, cost, wide_inf, -wide_inf) == *widened);

    // A tiny reachable negative cycle plus isolated vertices must not rescan V^2 rows.
    nidx_t row_calls = 0;
    vector<edge> loop{{0, -1}}, empty;
    auto isolated = ngraph{1000, [&](nidx_t v) -> auto& {
        ++row_calls;
        return v == 0 ? loop : empty;
    }, [](const edge& item) { return item.to; }};
    CHECK(!nbellman_ford(isolated, 0, cost, inf));
    CHECK(row_calls <= 3000);
    row_calls = 0;
    auto isolated_closure = nbellman_ford_closure(isolated, 0, cost, inf, neg_inf);
    CHECK(isolated_closure[0] == neg_inf);
    for (nidx_t v = 1; v < 1000; ++v) CHECK(isolated_closure[v] == inf);
    CHECK(row_calls <= 3000);
}
