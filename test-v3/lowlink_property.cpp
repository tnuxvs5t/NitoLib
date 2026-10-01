#include "../src-v3/graph_algo.hpp"
#include "../src-v3/graph_store.hpp"
#include "../src-v3/hash.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using edge = pair<nidx_t, nidx_t>;
struct incidence { nidx_t from, to, id; };

// Independent oracle: remove one vertex/individual edge, then count components by BFS.
nidx_t components(nidx_t n, const vector<edge>& edges, nidx_t removed_vertex = -1,
                  nidx_t removed_edge = -1) {
    vector<vector<nidx_t>> adjacency(n);
    for (nidx_t i = 0; i < nidx_t(edges.size()); ++i) {
        auto [u, v] = edges[i];
        if (i == removed_edge || u == removed_vertex || v == removed_vertex) continue;
        adjacency[u].push_back(v);
        adjacency[v].push_back(u);
    }
    vector<unsigned char> seen(n);
    nidx_t count = 0;
    for (nidx_t start = 0; start < n; ++start) if (start != removed_vertex && !seen[start]) {
        ++count;
        vector<nidx_t> queue{start};
        seen[start] = true;
        for (nidx_t at = 0; at < nidx_t(queue.size()); ++at)
            for (nidx_t to : adjacency[queue[at]]) if (!seen[to]) {
                seen[to] = true;
                queue.push_back(to);
            }
    }
    return count;
}

template <class G>
void verify(G&& graph, nidx_t n, const vector<edge>& edges, const vector<nidx_t>& ids) {
    auto result = nlowlink(graph);
    nidx_t baseline = components(n, edges);
    CHECK(nidx_t(result.articulation.size()) == n);
    for (nidx_t pos = 0; pos < n; ++pos)
        CHECK(result.articulation[pos] == (components(n, edges, graph.vertices[pos]) > baseline));
    vector<nidx_t> expected, actual = result.bridges;
    for (nidx_t i = 0; i < nidx_t(edges.size()); ++i)
        if (components(n, edges, -1, i) > baseline) expected.push_back(ids[i]);
    sort(expected.begin(), expected.end());
    sort(actual.begin(), actual.end());
    CHECK(actual == expected); // Also rejects duplicate bridge reports.
}

void check(nidx_t n, const vector<edge>& edges, mt19937& rng) {
    vector<vector<incidence>> adjacency(n);
    vector<incidence> arcs;
    vector<nidx_t> ids(edges.size());
    for (nidx_t i = 0; i < nidx_t(ids.size()); ++i)
        ids[i] = i ? numeric_limits<nidx_t>::max() - 17 * i : 0;
    shuffle(ids.begin(), ids.end(), rng);
    for (nidx_t i = 0; i < nidx_t(edges.size()); ++i) {
        auto [u, v] = edges[i];
        adjacency[u].push_back({u, v, ids[i]});
        adjacency[v].push_back({v, u, ids[i]});
        arcs.push_back({u, v, ids[i]});
        arcs.push_back({v, u, ids[i]});
    }
    for (auto& row : adjacency) shuffle(row.begin(), row.end(), rng);
    shuffle(arcs.begin(), arcs.end(), rng);
    auto target = [](const incidence& e) { return e.to; };
    auto identity = [](const incidence& e) { return e.id; };
    auto graph = ngraph{n, [&](nidx_t v) -> const auto& { return adjacency[v]; }, target, identity};
    verify(graph, n, edges, ids);
    // Reversed positions and independently owned temporary adjacency ranges.
    auto reversed = ngraph{nreverse(nrange(n)), [&](nidx_t v) { return adjacency[v]; }, target, identity};
    verify(reversed, n, edges, ids);
    auto csr = nmake_csr(n, arcs, [](const incidence& e) { return e.from; }, target, identity);
    verify(csr, n, edges, ids);
    verify(csr.view(), n, edges, ids);
    verify(as_const(csr).view(), n, edges, ids);
    auto expanded = nmake_undirected_csr(n, edges, [](const edge& e) { return e.first; },
                                       [](const edge& e) { return e.second; });
    iota(ids.begin(), ids.end(), nidx_t{0});
    verify(expanded, n, edges, ids);
}

int main() {
    mt19937 rng(0x10A11);
    check(0, {}, rng);
    check(5, {}, rng);
    check(1, {{0, 0}}, rng);
    check(2, {{0, 1}}, rng); // A DFS root with one child is not a cut vertex.
    check(2, {{0, 1}, {0, 1}}, rng); // Skip the entering ID, not every edge to the parent.
    check(5, {{0, 1}, {0, 2}, {0, 3}, {0, 4}}, rng); // Root child rule.
    check(4, {{0, 1}, {1, 2}, {2, 3}}, rng);
    check(3, {{0, 1}, {1, 2}, {2, 0}}, rng);
    // low[child] == dfn[parent]: parent is a cut vertex, tree edge is not a bridge.
    check(7, {{0, 1}, {1, 2}, {2, 3}, {3, 1}, {4, 5}, {4, 5}, {5, 5}}, rng);
    for (nidx_t round = 0; round < 600; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 10), m = nidx_t(rng() % 26);
        vector<edge> edges;
        for (nidx_t i = 0; i < m; ++i)
            edges.emplace_back(nidx_t(rng() % n), nidx_t(rng() % n));
        check(n, edges, rng);
    }
    vector<string> keys{"tail", "center", "leaf"};
    struct record { string to; nidx_t id; unique_ptr<int> payload; };
    unordered_map<string, vector<record>> adjacency;
    auto add = [&](string from, string to, nidx_t id) {
        adjacency[from].push_back({to, id, make_unique<int>(0)});
    };
    add("tail", "center", 71); add("center", "tail", 71);
    add("center", "leaf", 8); add("center", "leaf", 9);
    add("leaf", "center", 9); add("leaf", "center", 8);
    add("center", "center", numeric_limits<nidx_t>::max());
    auto named = ngraph{ninvert(nall(keys)),
        [token = make_unique<int>(0), &adjacency](const string& key) -> const auto& {
            return adjacency.at(key);
        }, [](const record& e) -> const string& { return e.to; },
        [token = make_unique<int>(0)](const record& e) { return e.id + *token; }};
    auto result = nlowlink(named); // Move-only port, semantic keys, single-entry loop.
    CHECK((result.articulation == vector<unsigned char>{0, 1, 0}));
    CHECK((result.bridges == vector<nidx_t>{71}));
    cout << "lowlink: edge-ID deletion oracle, 600 multigraphs, sparse IDs, CSR/views/named ports OK\n";
}
