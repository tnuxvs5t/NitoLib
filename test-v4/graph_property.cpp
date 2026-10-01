#include "../src-v4/graph_algo.hpp"
#include "../src-v4/graph_store.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct edge {
    nidx_t from, to, weight, id;
};

int main() {
    vector<vector<nidx_t>> dag{{1, 2}, {3}, {3}, {}};
    auto graph = ngraph{nrange(4), [&](nidx_t v) -> auto& { return dag[v]; }};
    CHECK((ndijkstra(graph, 0, [](nidx_t) { return nidx_t(1); }, 1'000'000) ==
           vector<nidx_t>{0, 1, 1, 2}));
    CHECK((n01bfs(graph, 0, [](nidx_t) { return nidx_t(1); }) ==
           vector<nidx_t>{0, 1, 1, 2}));
    CHECK(ntoposort(graph).size() == 4);

    vector<vector<nidx_t>> weighted{{1, 2}, {2}, {3}, {}};
    auto signed_graph = ngraph{nrange(4), [&](nidx_t v) -> auto& { return weighted[v]; }};
    auto bellman = nbellman_ford(signed_graph, 0,
                                 [](nidx_t to) { return to == 2 ? -4 : 2; }, 1'000'000);
    CHECK(bellman.has_value() && (*bellman)[3] == -2);
    vector<vector<pair<nidx_t, nidx_t>>> cycle{{{1, 1}}, {{2, -3}}, {{1, 1}}};
    auto negative = ngraph{nrange(3), [&](nidx_t v) -> auto& { return cycle[v]; },
                           [](const auto& e) { return e.first; }};
    CHECK(!nbellman_ford(negative, 0, [](const auto& e) { return e.second; }, 1'000'000));

    vector<vector<nidx_t>> scc_forward{{1}, {2}, {0, 3}, {}};
    vector<vector<nidx_t>> scc_reverse{{2}, {0}, {1}, {2}};
    auto forward_graph = ngraph{nrange(4), [&](nidx_t v) -> auto& { return scc_forward[v]; }};
    auto reverse_graph = ngraph{nrange(4), [&](nidx_t v) -> auto& { return scc_reverse[v]; }};
    auto components = nscc(forward_graph, reverse_graph);
    CHECK(components.count == 2 && components.component[0] == components.component[1] &&
          components.component[1] == components.component[2] &&
          components.component[3] != components.component[0]);

    vector<edge> records{
        {0, 1, 4, 10}, {1, 0, 4, 10}, {1, 2, 3, 11}, {2, 1, 3, 11},
        {0, 2, 9, 12}, {2, 0, 9, 12}
    };
    auto csr = nmake_csr(3, records, [](const edge& e) { return e.from; },
                         [](const edge& e) { return e.to; },
                         [](const edge& e) { return e.id; });
    CHECK(csr.edges(0).len() == 2 && csr.edge_id(csr.edges(0)[0]) == 10);
    auto csr_view = csr.view();
    CHECK((ndijkstra(csr_view, 0, [](const edge& e) { return e.weight; }, 1'000'000) ==
           vector<nidx_t>{0, 4, 7}));

    vector<edge> undirected{{0, 1, 1, 0}, {1, 2, 1, 1}, {1, 3, 1, 2}};
    auto tree = nmake_undirected_csr(4, undirected,
                                     [](const edge& e) { return e.from; },
                                     [](const edge& e) { return e.to; });
    auto cuts = nlowlink(tree);
    CHECK(cuts.articulation[1] && !cuts.articulation[0]);
    CHECK(cuts.bridges.size() == 3);
    auto blockcut = nblockcut(tree);
    CHECK(blockcut.original_size == 4 && blockcut.adjacency.size() == 7);

    vector<vector<pair<nidx_t, nidx_t>>> euler_adj(3);
    euler_adj[0] = {{1, 0}, {2, 2}};
    euler_adj[1] = {{0, 0}, {2, 1}};
    euler_adj[2] = {{1, 1}, {0, 2}};
    auto euler_graph = ngraph{nrange(3), [&](nidx_t v) -> auto& { return euler_adj[v]; },
                              [](const auto& e) { return e.first; },
                              [](const auto& e) { return e.second; }};
    auto euler = neuler(euler_graph, 3, neuler_kind::undirected);
    CHECK(euler.complete && euler.edges.size() == 3 && euler.vertices.size() == 4);

    auto prim = ndenseprim(4, [](nidx_t a, nidx_t b) {
        if (a == b) return nidx_t(1'000'000);
        return nidx_t(abs(a - b));
    }, nidx_t(1'000'000));
    CHECK(prim.components == 1 && prim.weight == 3);

    cout << "v4 graph: CSR ports, shortest paths, SCC, Euler and lowlink passed\n";
}
