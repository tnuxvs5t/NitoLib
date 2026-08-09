#include "../src-v3/dynamic_tree.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    mt19937 rng(0xE77);
    for (int round = 0; round < 500; ++round) {
        int n = 1 + int(rng() % 35);
        vector<long long> value(n);
        for (long long& x : value) x = int(rng() % 101) - 50;
        nett_forest<long long> forest(nall(value));
        vector<set<int>> adjacency(n);
        set<pair<int, int>> edges;

        auto component = [&](int source) {
            vector<int> vertices{source};
            vector<unsigned char> seen(n);
            seen[source] = true;
            for (int at = 0; at < int(vertices.size()); ++at)
                for (int to : adjacency[vertices[at]])
                    if (!seen[to]) seen[to] = true, vertices.push_back(to);
            return vertices;
        };
        auto verify = [&] {
            for (int check = 0; check < 20; ++check) {
                int vertex = int(rng() % n);
                auto vertices = component(vertex);
                long long sum = 0;
                for (int x : vertices) sum += value[x];
                CHECK(forest.component_size(vertex) == int(vertices.size()));
                CHECK(forest.fold(vertex) == sum);
                int other = int(rng() % n);
                CHECK(forest.connected(vertex, other) ==
                      (find(vertices.begin(), vertices.end(), other) != vertices.end()));
            }
        };

        for (int step = 0; step < 3000; ++step) {
            int operation = int(rng() % 4);
            if (operation == 0) {
                int vertex = int(rng() % n);
                value[vertex] = int(rng() % 201) - 100;
                forest.set(vertex, value[vertex]);
            } else if (operation <= 2) {
                int a = int(rng() % n), b = int(rng() % n);
                if (a == b) continue;
                if (a > b) swap(a, b);
                if (!forest.connected(a, b)) {
                    forest.link(a, b);
                    adjacency[a].insert(b);
                    adjacency[b].insert(a);
                    edges.emplace(a, b);
                }
            } else if (!edges.empty()) {
                auto it = edges.begin();
                advance(it, int(rng() % edges.size()));
                auto [a, b] = *it;
                forest.cut(a, b);
                adjacency[a].erase(b);
                adjacency[b].erase(a);
                edges.erase(it);
            }
            if (step % 31 == 0) verify();
        }
        verify();
    }

    int n = 5000;
    vector<long long> ones(n, 1);
    nett_forest<long long> chain(nall(ones));
    for (int vertex = 1; vertex < n; ++vertex) chain.link(vertex - 1, vertex);
    CHECK(chain.component_size(0) == n && chain.fold(n - 1) == n);
    for (int vertex = 1; vertex < n; vertex += 2) chain.cut(vertex - 1, vertex);
    CHECK(chain.component_size(0) == 1);
    for (int vertex = 2; vertex < n; vertex += 2)
        CHECK(chain.component_size(vertex) == 2);
}
