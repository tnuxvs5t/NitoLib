#include "../src-v3/graph_algo.hpp"
#include "../src-v3/graph_store.hpp"
#include "../src-v3/hash.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using edge = pair<nidx_t, nidx_t>;
using rows = vector<vector<nidx_t>>;
struct incidence { nidx_t from, to, id; };

vector<nidx_t> labels(const rows& adjacency, nidx_t removed = -1) {
    nidx_t n = nidx_t(adjacency.size()), count = 0;
    vector<nidx_t> component(n, -1);
    for (nidx_t start = 0; start < n; ++start) if (start != removed && component[start] < 0) {
        vector<nidx_t> queue{start};
        component[start] = count++;
        for (nidx_t at = 0; at < nidx_t(queue.size()); ++at)
            for (nidx_t to : adjacency[queue[at]])
                if (to != removed && component[to] < 0) {
                    component[to] = component[start];
                    queue.push_back(to);
                }
    }
    return component;
}

nidx_t component_count(const vector<nidx_t>& component) {
    nidx_t count = 0;
    for (nidx_t value : component) count = max(count, value + 1);
    return count;
}

// Independent exponential oracle: maximal induced subsets surviving any one deletion.
// Only the small oracle inputs (n <= 8) use masks; production positions use nidx_t.
rows oracle(const rows& adjacency) {
    nidx_t n = nidx_t(adjacency.size());
    auto connected = [&](unsigned mask) {
        if (!mask) return true;
        unsigned seen = mask & (~mask + 1);
        for (;;) {
            unsigned next = seen;
            for (nidx_t v = 0; v < n; ++v) if (seen & (1u << v))
                for (nidx_t to : adjacency[v]) next |= mask & (1u << to);
            if (next == seen) return seen == mask;
            seen = next;
        }
    };
    vector<unsigned> candidates;
    for (unsigned mask = 1; mask < (1u << n); ++mask) {
        if (!connected(mask)) continue;
        bool valid = true;
        for (nidx_t v = 0; v < n; ++v) if (mask & (1u << v))
            valid &= connected(mask ^ (1u << v));
        if (valid) candidates.push_back(mask);
    }
    rows blocks;
    for (unsigned mask : candidates) {
        bool maximal = true;
        for (unsigned other : candidates)
            if (mask != other && (mask & other) == mask) maximal = false;
        if (maximal) {
            vector<nidx_t> members;
            for (nidx_t v = 0; v < n; ++v) if (mask & (1u << v)) members.push_back(v);
            blocks.push_back(move(members));
        }
    }
    sort(blocks.begin(), blocks.end());
    return blocks;
}

template <class G>
void verify(G&& graph, const rows& original, const rows& expected,
            const vector<nidx_t>& position_to_vertex, const vector<edge>& edges,
            const vector<nidx_t>& ids) {
    auto result = nblockcut(graph);
    nidx_t n = nidx_t(original.size()), total = nidx_t(result.adjacency.size());
    CHECK(result.original_size == n);
    CHECK(n <= total && total <= 2 * n);
    CHECK(nidx_t(result.edges.size()) == total - n);
    const auto& forest = result.adjacency;
    nidx_t incidences = 0;
    for (nidx_t from = 0; from < total; ++from) {
        CHECK(!forest[from].empty());
        auto sorted = forest[from];
        sort(sorted.begin(), sorted.end());
        CHECK(adjacent_find(sorted.begin(), sorted.end()) == sorted.end());
        for (nidx_t to : forest[from]) {
            CHECK(0 <= to && to < total);
            CHECK((from < n) != (to < n));
            CHECK(count(forest[to].begin(), forest[to].end(), from) == 1);
            ++incidences;
        }
    }
    auto baseline = labels(original), tree_components = labels(forest);
    nidx_t components = component_count(baseline);
    CHECK(component_count(tree_components) == components);
    CHECK(incidences == 2 * (total - components)); // Together with symmetry: a forest.
    rows actual;
    vector<nidx_t> seen(edges.size());
    for (nidx_t block = n; block < total; ++block) {
        vector<nidx_t> members;
        for (nidx_t pos : forest[block]) members.push_back(position_to_vertex[pos]);
        sort(members.begin(), members.end());
        // Independent edge oracle: all non-loop input edges induced by this block.
        vector<nidx_t> expected_edges, actual_edges = result.edges[block - n];
        for (nidx_t i = 0; i < nidx_t(edges.size()); ++i) {
            auto [u, v] = edges[i];
            if (u != v && binary_search(members.begin(), members.end(), u)
                       && binary_search(members.begin(), members.end(), v)) {
                expected_edges.push_back(ids[i]);
                ++seen[i];
            }
        }
        sort(expected_edges.begin(), expected_edges.end());
        sort(actual_edges.begin(), actual_edges.end());
        CHECK(actual_edges == expected_edges);
        actual.push_back(move(members));
    }
    for (nidx_t i = 0; i < nidx_t(edges.size()); ++i)
        CHECK(seen[i] == (edges[i].first != edges[i].second));
    sort(actual.begin(), actual.end());
    CHECK(actual == expected);

    // Deleting any original vertex preserves exactly the same surviving connectivity.
    // On the verified forest this is also the unique-path separator property.
    for (nidx_t removed = -1; removed < n; ++removed) {
        auto source_labels = labels(original, removed < 0 ? -1 : position_to_vertex[removed]);
        auto forest_labels = labels(forest, removed);
        for (nidx_t u = 0; u < n; ++u) if (u != removed)
            for (nidx_t v = 0; v < n; ++v) if (v != removed)
                CHECK((source_labels[position_to_vertex[u]] == source_labels[position_to_vertex[v]])
                      == (forest_labels[u] == forest_labels[v]));
        if (removed >= 0)
            CHECK((forest[removed].size() > 1) == (component_count(source_labels) > components));
    }
    // The output composes with the existing graph port without another graph owner.
    auto view = ngraph{total, [&](nidx_t v) -> const auto& { return forest[v]; }};
    if (total) {
        auto distance = nbfs(view, nidx_t{0});
        for (nidx_t v = 0; v < total; ++v)
            CHECK((distance[v] >= 0) == (tree_components[v] == tree_components[0]));
    }
}

void check(nidx_t n, const vector<edge>& edges, mt19937& rng) {
    rows adjacency(n);
    vector<vector<incidence>> records(n);
    vector<incidence> arcs;
    vector<nidx_t> ids(edges.size());
    for (nidx_t i = 0; i < nidx_t(ids.size()); ++i)
        ids[i] = i ? numeric_limits<nidx_t>::max() - 17 * i : 0;
    shuffle(ids.begin(), ids.end(), rng);
    for (nidx_t i = 0; i < nidx_t(edges.size()); ++i) {
        auto [u, v] = edges[i];
        adjacency[u].push_back(v);
        adjacency[v].push_back(u);
        records[u].push_back({u, v, ids[i]});
        records[v].push_back({v, u, ids[i]});
        arcs.push_back({u, v, ids[i]});
        arcs.push_back({v, u, ids[i]});
    }
    for (auto& row : records) shuffle(row.begin(), row.end(), rng);
    shuffle(arcs.begin(), arcs.end(), rng);
    auto expected = oracle(adjacency);
    vector<nidx_t> positions(n);
    iota(positions.begin(), positions.end(), nidx_t{0});
    auto target = [](const incidence& e) { return e.to; };
    auto identity = [](const incidence& e) { return e.id; };
    auto graph = ngraph{n, [&](nidx_t v) -> const auto& { return records[v]; }, target, identity};
    verify(graph, adjacency, expected, positions, edges, ids);
    auto csr = nmake_csr(n, arcs, [](const incidence& e) { return e.from; }, target, identity);
    verify(csr, adjacency, expected, positions, edges, ids);
    verify(csr.view(), adjacency, expected, positions, edges, ids);
    verify(as_const(csr).view(), adjacency, expected, positions, edges, ids);
    reverse(positions.begin(), positions.end());
    // Nonidentity inverse, independent temporary ranges surviving nested DFS calls.
    auto reversed = ngraph{nreverse(nrange(n)), [&](nidx_t v) { return records[v]; }, target, identity};
    verify(reversed, adjacency, expected, positions, edges, ids);
    auto expanded = nmake_undirected_csr(n, edges, [](const edge& e) { return e.first; },
                                       [](const edge& e) { return e.second; });
    iota(ids.begin(), ids.end(), nidx_t{0});
    reverse(positions.begin(), positions.end());
    verify(expanded, adjacency, expected, positions, edges, ids);
}

int main() {
    mt19937 rng(0xB10CC07);
    check(0, {}, rng);
    check(5, {}, rng);
    check(1, {{0, 0}, {0, 0}}, rng);
    check(2, {{0, 1}}, rng);
    check(2, {{0, 1}, {0, 1}}, rng);
    check(5, {{0, 1}, {0, 2}, {0, 3}, {0, 4}}, rng);
    check(5, {{0, 1}, {1, 2}, {2, 3}, {3, 4}}, rng);
    check(3, {{0, 1}, {1, 2}, {2, 0}}, rng);
    check(5, {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 4}, {4, 2}}, rng);
    check(8, {{0, 1}, {1, 2}, {2, 3}, {3, 1}, {4, 5}, {4, 5}, {5, 5}, {6, 6}}, rng);
    for (nidx_t round = 0; round < 600; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 8), m = nidx_t(rng() % 26);
        vector<edge> edges;
        for (nidx_t i = 0; i < m; ++i)
            edges.emplace_back(nidx_t(rng() % n), nidx_t(rng() % n));
        check(n, edges, rng);
    }
    // Semantic keys, move-only port AND edge records, and single-entry self-loops.
    struct record { string to; nidx_t id; unique_ptr<int> payload; };
    vector<string> keys{"tail", "center", "leaf", "isolated"};
    rows original{{1}, {0, 2, 2, 1}, {1, 1}, {3}};
    vector<edge> edges{{0, 1}, {1, 2}, {1, 2}, {1, 1}, {3, 3}};
    vector<nidx_t> ids{71, 8, 9, 100, 101};
    unordered_map<string, vector<record>> adjacency;
    for (nidx_t i = 0; i < nidx_t(edges.size()); ++i) {
        auto [u, v] = edges[i];
        adjacency[keys[u]].push_back({keys[v], ids[i], make_unique<int>(0)});
        if (u != v) adjacency[keys[v]].push_back({keys[u], ids[i], make_unique<int>(0)});
    }
    auto named = ngraph{ninvert(nall(keys)),
        [token = make_unique<int>(0), &adjacency](const string& key) -> const auto& {
            return adjacency.at(key);
        }, [](const record& e) -> const string& { return e.to; },
        [token = make_unique<int>(0)](const record& e) { return e.id + *token; }};
    verify(named, original, oracle(original), {0, 1, 2, 3}, edges, ids);
    // Results contain owned positions and remain valid after the input is destroyed.
    auto detached = [] {
        vector<edge> temporary{{0, 1}, {1, 2}};
        return nblockcut(nmake_undirected_csr(3, temporary,
            [](const edge& e) { return e.first; }, [](const edge& e) { return e.second; }));
    }();
    CHECK(detached.original_size == 3 && detached.adjacency.size() == 5);
    CHECK(detached.adjacency[1].size() == 2);
    CHECK((detached.edges == rows{{1}, {0}}));
    cout << "blockcut: subset/deletion and edge-membership oracles, 600 multigraphs, sparse IDs/views OK\n";
}
