#include "../src-v4/graph_algo.hpp"
#include "../src-v4/graph_store.hpp"
#include "../src-v4/rooted.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x << '\n'; abort(); } } while (false)

struct arc { nidx_t to, id; };

static bool brute_euler(nidx_t n, const vector<pair<nidx_t, nidx_t>>& edges,
                        bool directed, nidx_t start) {
    if (edges.empty()) return true;
    auto dfs = [&](auto&& self, nidx_t v, unsigned used) -> bool {
        if (used == (1U << edges.size()) - 1) return true;
        for (size_t i = 0; i < edges.size(); ++i) if (!(used >> i & 1U)) {
            auto [a, b] = edges[i];
            if (v == a && self(self, b, used | (1U << i))) return true;
            if (!directed && v == b && self(self, a, used | (1U << i))) return true;
        }
        return false;
    };
    if (start >= 0) return dfs(dfs, start, 0);
    for (nidx_t v = 0; v < n; ++v) if (dfs(dfs, v, 0)) return true;
    return false;
}

static void check_euler(const vector<pair<nidx_t, nidx_t>>& edges,
                         const neuler_result& result, bool directed, nidx_t start) {
    if (!result.complete) return;
    CHECK(result.edges.size() == edges.size());
    CHECK(result.vertices.size() == edges.size() + 1);
    if (start >= 0) CHECK(result.vertices.front() == start);
    vector<bool> used(edges.size());
    for (size_t i = 0; i < edges.size(); ++i) {
        nidx_t id = (result.edges[i] - 17) / 11;
        CHECK(id >= 0 && size_t(id) < edges.size() && !used[id]);
        CHECK(result.edges[i] == 17 + 11 * id);
        used[id] = true;
        auto [a, b] = edges[id];
        nidx_t u = result.vertices[i], v = result.vertices[i + 1];
        CHECK((a == u && b == v) || (!directed && a == v && b == u));
    }
}

int main() {
    vector<string> keys{"leaf", "root", "mid", "isolate"};
    auto vertices = ninvert(nall(keys));
    vector<vector<string>> adjacency{{}, {"mid"}, {"leaf"}, {}};
    auto next = [&](const string& key) -> const auto& {
        return adjacency[vertices.inverse(key)];
    };
    auto graph = ngraph{vertices, next};
    CHECK(graph.edges(string("root")).front() == "mid");
    CHECK((nbfs(graph, string("root")) == vector<nidx_t>{2, 0, 1, -1}));
    CHECK((nbfs_many(graph, vector<string>{"root", "root", "isolate"}) ==
           vector<nidx_t>{2, 0, 1, 0}));
    CHECK((ndijkstra(graph, string("root"), [](const string&) { return 1; }, 99) ==
           vector<int>{2, 0, 1, 99}));
    CHECK((n01bfs(graph, string("root"), [](const string&) { return 1; }) ==
           vector<nidx_t>{2, 0, 1, -1}));
    auto rooted = nroot(graph, vector<string>{"root"});
    CHECK(rooted.depths()(string("leaf")) == 2 && rooted.par[3] == -1);

    // A fresh owning adjacency range must be acquired once per iterator pair.
    auto by_value = ngraph{vertices, [&](const string& key) { return next(key); }};
    auto bellman = nbellman_ford(by_value, string("root"), [](const string&) { return 1; }, 99);
    CHECK(bellman && (*bellman == vector<int>{2, 0, 1, 99}));

    vector<vector<arc>> fork{{{1, 17}, {2, 28}}, {}, {}};
    auto integral = ngraph{nidx_t(3), [&](nidx_t v) -> auto& { return fork[v]; },
                          [](arc e) { return e.to; }, [](arc e) { return e.id; }};
    static_assert(same_as<decltype(integral.vertices), nvertices>);
    CHECK(!neuler(integral, 2, neuler_kind::directed).complete);
    CHECK((nbfs(integral, nidx_t(0)) == vector<nidx_t>{0, 1, 1}));
    auto empty = ngraph{nidx_t(0), [](nidx_t) { return vector<nidx_t>{}; }};
    CHECK(nbfs_many(empty, vector<nidx_t>{}).empty());

    mt19937 rng(0x7347);
    for (int trial = 0; trial < 500; ++trial) {
        nidx_t n = 1 + nidx_t(rng() % 10);
        vector<nidx_t> labels(n);
        for (nidx_t i = 0; i < n; ++i) labels[i] = 100 + 7 * i;
        shuffle(labels.begin(), labels.end(), rng);
        auto domain = ninvert(nall(labels));
        vector<vector<nidx_t>> adj(n), rev(n);
        vector<vector<nidx_t>> distance(n, vector<nidx_t>(n, 99));
        for (nidx_t a = 0; a < n; ++a) {
            distance[a][a] = 0;
            for (nidx_t b = 0; b < n; ++b) if (rng() % 4 == 0) {
                adj[a].push_back(labels[b]);
                rev[b].push_back(labels[a]);
                distance[a][b] = min(distance[a][b], nidx_t(1));
            }
        }
        for (nidx_t k = 0; k < n; ++k)
            for (nidx_t a = 0; a < n; ++a)
                for (nidx_t b = 0; b < n; ++b)
                    distance[a][b] = min(distance[a][b], distance[a][k] + distance[k][b]);
        auto keyed = ngraph{domain, [&](nidx_t key) -> auto& { return adj[domain.inverse(key)]; }};
        auto reverse_domain = nreverse(domain);
        auto reversed = ngraph{reverse_domain, [&](nidx_t key) -> auto& { return rev[domain.inverse(key)]; }};
        auto components = nscc(keyed, reversed);
        for (nidx_t a = 0; a < n; ++a)
            for (nidx_t b = 0; b < n; ++b)
                CHECK((components.component[a] == components.component[b]) ==
                      (distance[a][b] < 99 && distance[b][a] < 99));
        nidx_t source = nidx_t(rng() % n);
        auto bfs = nbfs(keyed, labels[source]);
        auto many = nbfs_many(keyed, vector<nidx_t>{labels[source], labels[0], labels[source]});
        auto heap = ndijkstra(keyed, labels[source], [](nidx_t) { return nidx_t(1); }, nidx_t(99));
        auto binary = n01bfs(keyed, labels[source], [](nidx_t) { return 1; });
        for (nidx_t b = 0; b < n; ++b) {
            nidx_t expected = distance[source][b];
            CHECK(heap[b] == expected);
            CHECK(bfs[b] == (expected == 99 ? -1 : expected));
            CHECK(binary[b] == bfs[b]);
            expected = min(expected, distance[0][b]);
            CHECK(many[b] == (expected == 99 ? -1 : expected));
        }
    }

    for (int trial = 0; trial < 1200; ++trial) {
        nidx_t n = 1 + nidx_t(rng() % 5), m = nidx_t(rng() % 8);
        vector<pair<nidx_t, nidx_t>> edges(m);
        for (auto& [a, b] : edges) a = nidx_t(rng() % n), b = nidx_t(rng() % n);
        for (bool directed : {false, true}) {
            vector<vector<arc>> adj(n);
            for (nidx_t i = 0; i < m; ++i) {
                auto [a, b] = edges[i];
                adj[a].push_back({b, 17 + 11 * i});
                if (!directed) adj[b].push_back({a, 17 + 11 * i});
            }
            auto g = ngraph{n, [&](nidx_t v) -> auto& { return adj[v]; },
                            [](arc e) { return e.to; }, [](arc e) { return e.id; }};
            auto kind = directed ? neuler_kind::directed : neuler_kind::undirected;
            auto result = neuler(g, m, kind);
            CHECK(result.complete == brute_euler(n, edges, directed, -1));
            check_euler(edges, result, directed, -1);
            for (nidx_t start = 0; start < n; ++start) {
                result = neuler(g, start, m, kind);
                CHECK(result.complete == brute_euler(n, edges, directed, start));
                check_euler(edges, result, directed, start);
            }
        }
    }
    cout << "v4 graph contracts: keyed ports, BFS, owning adjacency, SCC and Euler oracle passed\n";
}
