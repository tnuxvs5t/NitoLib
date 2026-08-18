#include "../src-v3/graph.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct edge {
    int to;
    int ignored_weight;
};

int main() {
    mt19937 rng(0xA11CE);
    for (int round = 0; round < 5000; ++round) {
        int n = 1 + int(rng() % 50);
        vector<vector<int>> adjacency(n);
        vector<vector<edge>> records(n);
        for (int from = 0; from < n; ++from)
            for (int to = 0; to < n; ++to)
                if (rng() % 17 == 0) {
                    adjacency[from].push_back(to);
                    records[from].push_back({to, int(rng())});
                }
        int source = int(rng() % n);
        auto plain = ngraph{nrange(n), [&](int from) -> auto& { return adjacency[from]; }};
        auto packed = ngraph{nrange(n), [&](int from) -> auto& { return records[from]; },
                             [](const edge& item) { return item.to; }};
        auto a = nbfs(plain, source);
        auto b = nbfs(packed, source);
        CHECK(a == b);

        vector<int> expected(n, -1), queue{source};
        expected[source] = 0;
        for (int at = 0; at < int(queue.size()); ++at)
            for (int to : adjacency[queue[at]])
                if (expected[to] < 0)
                    expected[to] = expected[queue[at]] + 1, queue.push_back(to);
        CHECK(a == expected);
    }

    for (int round = 0; round < 3000; ++round) {
        int n = 1 + int(rng() % 100);
        vector<vector<int>> adjacency(n);
        vector<int> actual_parent(n);
        actual_parent[0] = 0;
        for (int vertex = 1; vertex < n; ++vertex) {
            int parent = int(rng() % vertex);
            actual_parent[vertex] = parent;
            adjacency[parent].push_back(vertex);
            adjacency[vertex].push_back(parent);
        }
        for (auto& edges : adjacency) shuffle(edges.begin(), edges.end(), rng);
        auto graph = ngraph{nrange(n), [&](int vertex) -> auto& { return adjacency[vertex]; }};
        auto tree = nroot(graph, nrange(1));
        auto parent = tree.parents();
        auto depth = tree.depths();
        auto subtree = tree.subtree_sizes();
        CHECK(tree.order().len() == n && tree.roots().len() == 1);
        vector<int> counted(n, 1);
        for (int vertex = n - 1; vertex > 0; --vertex)
            counted[actual_parent[vertex]] += counted[vertex];
        for (int vertex = 0; vertex < n; ++vertex) {
            CHECK(parent(vertex) == actual_parent[vertex]);
            CHECK(subtree(vertex) == counted[vertex]);
            if (vertex) CHECK(depth(vertex) == depth(actual_parent[vertex]) + 1);
            auto children = tree.children(vertex);
            for (int i = 0; i < children.len(); ++i)
                CHECK(parent(children[i]) == vertex);
        }
        auto child_graph = ngraph{tree.keys(), [&](int vertex) {
                                      return tree.children(vertex);
                                  }};
        auto tree_distance = nbfs(child_graph, 0);
        for (int vertex = 0; vertex < n; ++vertex)
            CHECK(tree_distance[vertex] == depth(vertex));
    }

    vector<string> names{"river", "gear", "cucumber", "lab"};
    unordered_map<string, int> id;
    for (int i = 0; i < int(names.size()); ++i) id[names[i]] = i;
    vector<vector<string>> named_edges{{"gear", "lab"}, {"cucumber"}, {"lab"}, {}};
    auto named = ngraph{ninvert(nall(names)), [&](const string& name) -> auto& {
                            return named_edges[id[name]];
                        }};
    auto named_distance = nbfs(named, string("river"));
    CHECK((named_distance == vector<int>{0, 1, 2, 1}));

    vector<vector<int>> hostile{{0, 1, 1}, {2, 0}, {1}, {4}, {3}};
    auto cyclic = ngraph{nrange(5), [&](int vertex) -> auto& { return hostile[vertex]; }};
    auto spanning = nroot(cyclic, nrange(5));
    vector<int> seen(5);
    for (int vertex : spanning.order()) ++seen[vertex];
    CHECK((seen == vector<int>{1, 1, 1, 1, 1}));
    CHECK(spanning.components()(0) == 0 && spanning.components()(2) == 0);
    CHECK(spanning.components()(3) == 3 && spanning.components()(4) == 3);

    int rows = 17, columns = 13;
    auto cells = nproduct(nrange(rows), nrange(columns));
    auto grid = ngraph{move(cells), [=](pair<int, int> cell) {
        vector<pair<int, int>> answer;
        for (auto [dx, dy] : {pair{-1, 0}, pair{1, 0}, pair{0, -1}, pair{0, 1}}) {
            int x = cell.first + dx, y = cell.second + dy;
            if (0 <= x && x < rows && 0 <= y && y < columns) answer.emplace_back(x, y);
        }
        return answer;
    }};
    auto distance = nbfs(grid, pair{0, 0});
    for (int x = 0; x < rows; ++x)
        for (int y = 0; y < columns; ++y)
            CHECK(distance[x * columns + y] == x + y);

    auto move_graph = ngraph{nrange(4), [token = make_unique<int>(1)](int vertex) {
        vector<int> edges;
        if (vertex + *token < 4) edges.push_back(vertex + *token);
        return edges;
    }};
    CHECK((nbfs(move(move_graph), 0) == vector<int>{0, 1, 2, 3}));
}
