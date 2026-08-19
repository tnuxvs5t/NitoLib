#include "../src-v3/graph.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct edge {
    nidx_t to;
    nidx_t ignored_weight;
};

int main() {
    mt19937 rng(0xA11CE);
    for (nidx_t round = 0; round < 5000; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 50);
        vector<vector<nidx_t>> adjacency(n);
        vector<vector<edge>> records(n);
        for (nidx_t from = 0; from < n; ++from)
            for (nidx_t to = 0; to < n; ++to)
                if (rng() % 17 == 0) {
                    adjacency[from].push_back(to);
                    records[from].push_back({to, nidx_t(rng())});
                }
        nidx_t source = nidx_t(rng() % n);
        auto plain = ngraph{nrange(n), [&](nidx_t from) -> auto& { return adjacency[from]; }};
        auto packed = ngraph{nrange(n), [&](nidx_t from) -> auto& { return records[from]; },
                             [](const edge& item) { return item.to; }};
        auto a = nbfs(plain, source);
        auto b = nbfs(packed, source);
        CHECK(a == b);

        vector<nidx_t> expected(n, -1), queue{source};
        expected[source] = 0;
        for (nidx_t at = 0; at < nidx_t(queue.size()); ++at)
            for (nidx_t to : adjacency[queue[at]])
                if (expected[to] < 0)
                    expected[to] = expected[queue[at]] + 1, queue.push_back(to);
        CHECK(a == expected);
    }

    for (nidx_t round = 0; round < 3000; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 100);
        vector<vector<nidx_t>> adjacency(n);
        vector<nidx_t> actual_parent(n);
        actual_parent[0] = 0;
        for (nidx_t vertex = 1; vertex < n; ++vertex) {
            nidx_t parent = nidx_t(rng() % vertex);
            actual_parent[vertex] = parent;
            adjacency[parent].push_back(vertex);
            adjacency[vertex].push_back(parent);
        }
        for (auto& edges : adjacency) shuffle(edges.begin(), edges.end(), rng);
        auto graph = ngraph{nrange(n), [&](nidx_t vertex) -> auto& { return adjacency[vertex]; }};
        auto tree = nroot(graph, nrange(1));
        auto parent = tree.parents();
        auto depth = tree.depths();
        auto subtree = tree.subtree_sizes();
        CHECK(tree.order().len() == n && tree.roots().len() == 1);
        vector<nidx_t> counted(n, 1);
        for (nidx_t vertex = n - 1; vertex > 0; --vertex)
            counted[actual_parent[vertex]] += counted[vertex];
        for (nidx_t vertex = 0; vertex < n; ++vertex) {
            CHECK(parent(vertex) == actual_parent[vertex]);
            CHECK(subtree(vertex) == counted[vertex]);
            if (vertex) CHECK(depth(vertex) == depth(actual_parent[vertex]) + 1);
            auto children = tree.children(vertex);
            for (nidx_t i = 0; i < children.len(); ++i)
                CHECK(parent(children[i]) == vertex);
        }
        auto child_graph = ngraph{tree.keys(), [&](nidx_t vertex) {
                                      return tree.children(vertex);
                                  }};
        auto tree_distance = nbfs(child_graph, 0);
        for (nidx_t vertex = 0; vertex < n; ++vertex)
            CHECK(tree_distance[vertex] == depth(vertex));
    }

    vector<string> names{"river", "gear", "cucumber", "lab"};
    unordered_map<string, nidx_t> id;
    for (nidx_t i = 0; i < nidx_t(names.size()); ++i) id[names[i]] = i;
    vector<vector<string>> named_edges{{"gear", "lab"}, {"cucumber"}, {"lab"}, {}};
    auto named = ngraph{ninvert(nall(names)), [&](const string& name) -> auto& {
                            return named_edges[id[name]];
                        }};
    auto named_distance = nbfs(named, string("river"));
    CHECK((named_distance == vector<nidx_t>{0, 1, 2, 1}));

    vector<vector<nidx_t>> hostile{{0, 1, 1}, {2, 0}, {1}, {4}, {3}};
    auto cyclic = ngraph{nrange(5), [&](nidx_t vertex) -> auto& { return hostile[vertex]; }};
    auto spanning = nroot(cyclic, nrange(5));
    vector<nidx_t> seen(5);
    for (nidx_t vertex : spanning.order()) ++seen[vertex];
    CHECK((seen == vector<nidx_t>{1, 1, 1, 1, 1}));
    CHECK(spanning.components()(0) == 0 && spanning.components()(2) == 0);
    CHECK(spanning.components()(3) == 3 && spanning.components()(4) == 3);

    nidx_t rows = 17, columns = 13;
    auto cells = nproduct(nrange(rows), nrange(columns));
    auto grid = ngraph{move(cells), [=](pair<nidx_t, nidx_t> cell) {
        vector<pair<nidx_t, nidx_t>> answer;
        for (auto [dx, dy] : {pair{-1, 0}, pair{1, 0}, pair{0, -1}, pair{0, 1}}) {
            nidx_t x = cell.first + dx, y = cell.second + dy;
            if (0 <= x && x < rows && 0 <= y && y < columns) answer.emplace_back(x, y);
        }
        return answer;
    }};
    auto distance = nbfs(grid, pair{0, 0});
    for (nidx_t x = 0; x < rows; ++x)
        for (nidx_t y = 0; y < columns; ++y)
            CHECK(distance[x * columns + y] == x + y);

    auto move_graph = ngraph{nrange(4), [token = make_unique<nidx_t>(1)](nidx_t vertex) {
        vector<nidx_t> edges;
        if (vertex + *token < 4) edges.push_back(vertex + *token);
        return edges;
    }};
    CHECK((nbfs(move(move_graph), 0) == vector<nidx_t>{0, 1, 2, 3}));
}
