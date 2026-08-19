#include "../src-v3/graph_algo.hpp"
#include "../src-v3/graph_store.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct edge {
    nidx_t from, to, weight;
    edge() = delete;
    edge(nidx_t source, nidx_t target, nidx_t cost) : from(source), to(target), weight(cost) {}
};

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
                    records.emplace_back(from, to, weight);
                    reverse_records.emplace_back(to, from, weight);
                    adjacency[from].emplace_back(from, to, weight);
                    reverse_adjacency[to].emplace_back(to, from, weight);
                }
        auto csr = nmake_csr(n, nall(records), [](const edge& item) { return item.from; },
                             [](const edge& item) { return item.to; });
        auto reverse_csr = nmake_csr(n, nall(reverse_records),
                                     [](const edge& item) { return item.from; },
                                     [](const edge& item) { return item.to; });
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
                      bucket[i].weight == adjacency[vertex][i].weight);
        }
    }

    vector<edge> records;
    auto empty = nmake_csr(5, nall(records), [](const edge& item) { return item.from; },
                           [state = make_unique<nidx_t>()](const edge& item) { return item.to + *state; });
    CHECK(nbfs(move(empty), 3) == vector<nidx_t>({-1, -1, -1, 0, -1}));
}
