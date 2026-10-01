#include "../src-v3/view.hpp"
#include "../src-v3/graph_algo.hpp"
#include "../src-v3/graph_store.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct edge {
    nidx_t from, to, weight, id;
    edge() = delete;
    edge(nidx_t source, nidx_t target, nidx_t cost, nidx_t identity)
        : from(source), to(target), weight(cost), id(identity) {}
};

template <class G, class E>
constexpr bool has_id = requires(G& graph, E& e) { graph.edge_id(e); };

int main() {
    mt19937 rng(0xC52);
    for (nidx_t round = 0; round < 5000; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 60);
        vector<edge> records, reverse_records;
        vector<vector<edge>> adjacency(n), reverse_adjacency(n);
        for (nidx_t from = 0; from < n; ++from)
            for (nidx_t to = 0; to < n; ++to)
                if (rng() % 13 == 0) {
                    nidx_t weight = nidx_t(rng() % 30);
                    nidx_t id = nidx_t(records.size()) * 7 + 19;
                    records.emplace_back(from, to, weight, id);
                    reverse_records.emplace_back(to, from, weight, id);
                    adjacency[from].emplace_back(from, to, weight, id);
                    reverse_adjacency[to].emplace_back(to, from, weight, id);
                }
        auto csr = nmake_csr(n, nall(records), [](const edge& item) { return item.from; },
                             [](const edge& item) { return item.to; },
                             [](const edge& item) { return item.id; });
        auto reverse_csr = nmake_csr(n, nall(reverse_records),
                                     [](const edge& item) { return item.from; },
                                     [](const edge& item) { return item.to; });
        auto view = csr.view();
        auto const_view = as_const(csr).view();
        static_assert(has_id<decltype(csr), edge> && has_id<decltype(view), edge>);
        static_assert(!has_id<decltype(reverse_csr), edge>);
        static_assert(!has_id<decltype(reverse_csr.view()), edge>);
        static_assert(!has_id<decltype(as_const(reverse_csr).view()), edge>);
        auto graph = ngraph{nrange(n), [&](nidx_t vertex) -> auto& { return adjacency[vertex]; },
                            [](const edge& item) { return item.to; }};
        auto reverse_graph = ngraph{nrange(n),
                                    [&](nidx_t vertex) -> auto& { return reverse_adjacency[vertex]; },
                                    [](const edge& item) { return item.to; }};
        nidx_t source = nidx_t(rng() % n);
        CHECK(nbfs(csr, source) == nbfs(graph, source));
        CHECK(ndijkstra(csr, source, [](const edge& item) { return item.weight; }, nidx_t(1e9)) ==
              ndijkstra(graph, source, [](const edge& item) { return item.weight; }, nidx_t(1e9)));
        auto a = nscc(csr, reverse_csr);
        auto b = nscc(graph, reverse_graph);
        for (nidx_t x = 0; x < n; ++x)
            for (nidx_t y = 0; y < n; ++y)
                CHECK((a.component[x] == a.component[y]) ==
                      (b.component[x] == b.component[y]));
        for (nidx_t vertex = 0; vertex < n; ++vertex) {
            auto bucket = csr.edges(vertex);
            CHECK(bucket.len() == nidx_t(adjacency[vertex].size()));
            for (nidx_t i = 0; i < bucket.len(); ++i)
                CHECK(bucket[i].from == adjacency[vertex][i].from &&
                      bucket[i].to == adjacency[vertex][i].to &&
                      bucket[i].weight == adjacency[vertex][i].weight &&
                      csr.edge_id(bucket[i]) == adjacency[vertex][i].id &&
                      view.edge_id(bucket[i]) == adjacency[vertex][i].id &&
                      const_view.edge_id(bucket[i]) == adjacency[vertex][i].id);
        }
    }

    // Real CSR reordering, move-only projections and views of a const owner.
    vector<edge> scrambled{{2, 1, 5, 81}, {0, 2, 7, 14}, {1, 0, 9, 93}, {0, 1, 2, 71}};
    auto owned = nmake_csr(3, scrambled, [](const edge& e) { return e.from; },
        [state = make_unique<int>(0)](const edge& e) mutable { return e.to + *state; },
        [state = make_unique<int>(0)](const edge& e) mutable { return e.id + *state; });
    auto moved = move(owned); // Create views only AFTER moving the owner.
    CHECK(moved.storage[0].id == 14 && moved.storage[1].id == 71);
    auto moved_view = as_const(moved).view();
    for (nidx_t v = 0; v < 3; ++v)
        for (const auto& e : moved_view.edges(v)) {
            CHECK(moved_view.edge_id(e) == e.id);
            CHECK(moved_view.target(e) == e.to);
        }
    CHECK((nbfs(moved_view, 0) == vector<nidx_t>{0, 1, 1}));

    // Undirected construction stores input positions, even for move-only payloads.
    struct input { nidx_t u, v; unique_ptr<int> payload; };
    vector<input> input_edges;
    input_edges.push_back({2, 0, make_unique<int>(7)});
    input_edges.push_back({0, 2, make_unique<int>(3)});
    input_edges.push_back({1, 1, make_unique<int>(11)});
    auto undirected = nmake_undirected_csr(4, input_edges,
        [](const input& e) { return e.u; }, [](const input& e) { return e.v; });
    vector<nidx_t> counts(3);
    for (nidx_t v = 0; v < 4; ++v)
        for (const auto& e : undirected.edges(v)) {
            nidx_t id = undirected.edge_id(e), to = undirected.target(e);
            CHECK(0 <= id && id < 3);
            const auto& original = input_edges[id];
            CHECK((v == original.u && to == original.v) || (v == original.v && to == original.u));
            CHECK(original.payload != nullptr);
            ++counts[id];
        }
    CHECK((counts == vector<nidx_t>{2, 2, 2}));
    CHECK(undirected.edges(3).len() == 0);
    CHECK(ndijkstra(undirected, 2, [&](const auto& e) { return *input_edges[e.id].payload; }, 100)
          == vector<int>({3, 100, 0, 100}));

    vector<edge> records;
    auto empty = nmake_csr(5, nall(records), [](const edge& item) { return item.from; },
                           [state = make_unique<nidx_t>()](const edge& item) { return item.to + *state; });
    CHECK(nbfs(move(empty), 3) == vector<nidx_t>({-1, -1, -1, 0, -1}));
}
