#include "../src-v3/graph_algo.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct edge {
    int to;
    long long weight;
};

int main() {
    mt19937 rng(0x6A4F);
    constexpr long long inf = (1LL << 60);
    for (int round = 0; round < 4000; ++round) {
        int n = 1 + int(rng() % 28);
        vector<vector<edge>> adjacency(n);
        vector<vector<int>> plain(n), reversed(n);
        vector<vector<long long>> distance(n, vector<long long>(n, inf));
        for (int vertex = 0; vertex < n; ++vertex) distance[vertex][vertex] = 0;
        for (int from = 0; from < n; ++from)
            for (int to = 0; to < n; ++to)
                if (rng() % 8 == 0) {
                    long long weight = rng() % 31;
                    adjacency[from].push_back({to, weight});
                    plain[from].push_back(to);
                    reversed[to].push_back(from);
                    distance[from][to] = min(distance[from][to], weight);
                }
        for (int middle = 0; middle < n; ++middle)
            for (int from = 0; from < n; ++from)
                for (int to = 0; to < n; ++to)
                    distance[from][to] = min(distance[from][to],
                                             distance[from][middle] + distance[middle][to]);

        auto weighted = ngraph{nrange(n), [&](int vertex) -> auto& { return adjacency[vertex]; },
                               [](const edge& item) { return item.to; }};
        int source = int(rng() % n);
        auto got = ndijkstra(weighted, source,
                             [](const edge& item) { return item.weight; }, inf);
        CHECK(got == distance[source]);

        auto graph = ngraph{nrange(n), [&](int vertex) -> auto& { return plain[vertex]; }};
        auto reverse_graph = ngraph{nrange(n),
                                    [&](int vertex) -> auto& { return reversed[vertex]; }};
        auto result = nscc(graph, reverse_graph);
        vector<vector<unsigned char>> reachable(n, vector<unsigned char>(n));
        for (int from = 0; from < n; ++from)
            for (int to = 0; to < n; ++to)
                reachable[from][to] = distance[from][to] < inf;
        vector<int> labels = result.component;
        CHECK(*min_element(labels.begin(), labels.end()) == 0);
        CHECK(*max_element(labels.begin(), labels.end()) + 1 == result.count);
        for (int a = 0; a < n; ++a)
            for (int b = 0; b < n; ++b)
                CHECK((labels[a] == labels[b]) == (reachable[a][b] && reachable[b][a]));

        auto order = ntoposort(graph);
        if (int(order.size()) == n) {
            vector<int> position(n);
            for (int i = 0; i < n; ++i) position[order[i]] = i;
            for (int from = 0; from < n; ++from)
                for (int to : plain[from]) CHECK(position[from] < position[to]);
        } else {
            bool has_nontrivial = false;
            for (int a = 0; a < n; ++a)
                for (int b = 0; b < n; ++b)
                    if (a != b && labels[a] == labels[b]) has_nontrivial = true;
            bool self_loop = false;
            for (int vertex = 0; vertex < n; ++vertex)
                self_loop |= find(plain[vertex].begin(), plain[vertex].end(), vertex) != plain[vertex].end();
            CHECK(has_nontrivial || self_loop);
        }
    }

    vector<string> keys{"s", "a", "b", "t"};
    unordered_map<string, int> id{{"s", 0}, {"a", 1}, {"b", 2}, {"t", 3}};
    struct named_edge { string to; int cost; };
    vector<vector<named_edge>> edges{{{"a", 4}, {"b", 1}}, {{"t", 2}},
                                     {{"a", 1}, {"t", 8}}, {}};
    auto named = ngraph{nall(keys), [&](const string& key) -> auto& { return edges[id[key]]; },
                        [](const named_edge& item) -> const string& { return item.to; }};
    auto named_distance = ndijkstra(named, string("s"),
                                    [](const named_edge& item) { return item.cost; },
                                    int(1e9), [&](const string& key) { return id[key]; });
    CHECK((named_distance == vector<int>{0, 2, 1, 4}));

    vector<vector<int>> pending_siblings{{1, 2}, {2}, {1}};
    vector<vector<int>> pending_reverse{{}, {0, 2}, {0, 1}};
    auto sibling_graph = ngraph{nrange(3),
                                [&](int vertex) -> auto& { return pending_siblings[vertex]; }};
    auto sibling_reverse = ngraph{nrange(3),
                                  [&](int vertex) -> auto& { return pending_reverse[vertex]; }};
    auto sibling_scc = nscc(sibling_graph, sibling_reverse);
    CHECK(sibling_scc.component[1] == sibling_scc.component[2]);
    CHECK(sibling_scc.component[0] != sibling_scc.component[1]);

    for (int round = 0; round < 2000; ++round) {
        int n = 1 + int(rng() % 35), source = int(rng() % n);
        vector<vector<edge>> adjacency(n);
        vector<int> oracle(n, int(1e9));
        oracle[source] = 0;
        for (int from = 0; from < n; ++from)
            for (int to = 0; to < n; ++to)
                if (rng() % 7 == 0) adjacency[from].push_back({to, int(rng() & 1U)});
        for (int iteration = 1; iteration < n; ++iteration)
            for (int from = 0; from < n; ++from) if (oracle[from] < int(1e9))
                for (auto item : adjacency[from])
                    oracle[item.to] = min(oracle[item.to], oracle[from] + int(item.weight));
        for (int& value : oracle) if (value == int(1e9)) value = -1;
        auto graph = ngraph{nrange(n), [&](int vertex) -> auto& { return adjacency[vertex]; },
                            [](const edge& item) { return item.to; }};
        CHECK(n01bfs(graph, source, [](const edge& item) { return int(item.weight); }) == oracle);
    }
}
