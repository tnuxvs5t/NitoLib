#include "../src-v3/graph_algo.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using W = long long;
constexpr W inf = 1LL << 60;

void check(const vector<vector<W>>& a, W infinity = inf) {
    nidx_t n = nidx_t(a.size());
    vector<vector<unsigned char>> queried(n, vector<unsigned char>(n));
    auto cost = [state = make_unique<nidx_t>(0), &a, &queried](nidx_t u, nidx_t v) mutable {
        CHECK(u != v && !queried[u][v] && !queried[v][u]);
        queried[u][v] = true;
        ++*state;
        return a[u][v];
    };
    auto result = ndenseprim(n, cost, infinity); // Borrow a move-only callable.

    // Independent Kruskal oracle, using explicit component relabeling instead of DSU.
    vector<tuple<W, nidx_t, nidx_t>> edges;
    vector<nidx_t> labels(n), actual(n);
    iota(labels.begin(), labels.end(), 0);
    iota(actual.begin(), actual.end(), 0);
    for (nidx_t u = 0; u < n; ++u)
        for (nidx_t v = u + 1; v < n; ++v) {
            CHECK(queried[u][v] || queried[v][u]);
            if (a[u][v] != infinity) edges.emplace_back(a[u][v], u, v);
        }
    sort(edges.begin(), edges.end());
    W expected = 0, selected = 0;
    nidx_t components = n, roots = 0;
    for (auto [w, u, v] : edges) if (labels[u] != labels[v]) {
        nidx_t old = labels[v], replacement = labels[u];
        for (auto& label : labels) if (label == old) label = replacement;
        expected += w;
        --components;
    }
    CHECK(nidx_t(result.parent.size()) == n);
    for (nidx_t v = 0; v < n; ++v) {
        nidx_t u = result.parent[v];
        if (u == -1) { ++roots; continue; }
        CHECK(0 <= u && u < n && u != v && a[u][v] != infinity);
        CHECK(actual[u] != actual[v]); // Selected edges must form a forest.
        nidx_t old = actual[v], replacement = actual[u];
        for (auto& label : actual) if (label == old) label = replacement;
        selected += a[u][v];
    }
    CHECK(result.weight == expected && selected == expected);
    CHECK(result.components == components && roots == components);
    for (nidx_t u = 0; u < n; ++u)
        for (nidx_t v = 0; v < n; ++v)
            CHECK((labels[u] == labels[v]) == (actual[u] == actual[v]));
}

int main() {
    check({});
    check({{-100}}); // Self-loops never reach the weight callback.
    check(vector<vector<W>>(4, vector<W>(4, inf)));
    check({{0, 2, 3}, {2, 0, 2}, {3, 2, 0}}); // Cut-edge weights, not path distances.
    check({{0, -5, inf, inf}, {-5, 0, inf, inf},
           {inf, inf, 0, 0}, {inf, inf, 0, 0}});
    check({{0, 9, 9}, {9, 0, 9}, {9, 9, 0}}, 10); // Total may exceed infinity.
    mt19937 rng(0xD35E);
    for (nidx_t round = 0; round < 600; ++round) {
        nidx_t n = nidx_t(rng() % 25);
        vector<vector<W>> a(n, vector<W>(n, inf));
        for (nidx_t u = 0; u < n; ++u)
            for (nidx_t v = u + 1; v < n; ++v)
                if (rng() % 4 != 0) a[u][v] = a[v][u] = W(rng() % 31) - 15;
        check(a);
    }
    // An implicit graph needs neither a matrix nor an edge list.
    auto implicit = ndenseprim(8, [](nidx_t u, nidx_t v) { return W(abs(u - v)); }, inf);
    CHECK(implicit.weight == 7 && implicit.components == 1);
    __int128_t large = __int128_t(1) << 70;
    auto wide = ndenseprim(4, [large](nidx_t, nidx_t) { return large; }, large * 2);
    CHECK(wide.weight == large * 3 && wide.components == 1);
    cout << "dense_prim: fixed boundaries, random Kruskal oracle, forest edges, implicit/wide weights OK\n";
}
