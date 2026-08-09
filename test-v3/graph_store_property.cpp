#include "../src-v3/graph_algo.hpp"
#include "../src-v3/graph_store.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct edge {
    int from, to, weight;
    edge() = delete;
    edge(int source, int target, int cost) : from(source), to(target), weight(cost) {}
};

int main() {
    mt19937 rng(0xC52);
    for (int round = 0; round < 5000; ++round) {
        int n = 1 + int(rng() % 60);
        vector<edge> records, reverse_records;
        vector<vector<edge>> adjacency(n), reverse_adjacency(n);
        for (int from = 0; from < n; ++from)
            for (int to = 0; to < n; ++to)
                if (rng() % 13 == 0) {
                    int weight = int(rng() % 30);
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
        auto graph = ngraph{nrange(n), [&](int vertex) -> auto& { return adjacency[vertex]; },
                            [](const edge& item) { return item.to; }};
        auto reverse_graph = ngraph{nrange(n),
                                    [&](int vertex) -> auto& { return reverse_adjacency[vertex]; },
                                    [](const edge& item) { return item.to; }};
        int source = int(rng() % n);
        CHECK(nbfs(csr, source) == nbfs(graph, source));
        CHECK(ndijkstra(csr, source, [](const edge& item) { return item.weight; }, int(1e9)) ==
              ndijkstra(graph, source, [](const edge& item) { return item.weight; }, int(1e9)));
        auto a = nscc(csr, reverse_csr);
        auto b = nscc(graph, reverse_graph);
        for (int x = 0; x < n; ++x)
            for (int y = 0; y < n; ++y)
                CHECK((a.component[x] == a.component[y]) ==
                      (b.component[x] == b.component[y]));
        for (int vertex = 0; vertex < n; ++vertex) {
            auto bucket = csr.edges(vertex);
            CHECK(bucket.len() == int(adjacency[vertex].size()));
            for (int i = 0; i < bucket.len(); ++i)
                CHECK(bucket[i].from == adjacency[vertex][i].from &&
                      bucket[i].to == adjacency[vertex][i].to &&
                      bucket[i].weight == adjacency[vertex][i].weight);
        }
    }

    vector<edge> records;
    auto empty = nmake_csr(5, nall(records), [](const edge& item) { return item.from; },
                           [state = make_unique<int>()](const edge& item) { return item.to + *state; });
    CHECK(nbfs(move(empty), 3) == vector<int>({-1, -1, -1, 0, -1}));
}
