#include "../src-v3/dynamic_tree.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    mt19937 rng(0xE77);
    for (nidx_t round = 0; round < 500; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 35);
        vector<long long> value(n);
        for (long long& x : value) x = nidx_t(rng() % 101) - 50;
        nett_forest<long long> forest(nall(value));
        vector<set<nidx_t>> adjacency(n);
        set<pair<nidx_t, nidx_t>> edges;

        auto component = [&](nidx_t source) {
            vector<nidx_t> vertices{source};
            vector<unsigned char> seen(n);
            seen[source] = true;
            for (nidx_t at = 0; at < nidx_t(vertices.size()); ++at)
                for (nidx_t to : adjacency[vertices[at]])
                    if (!seen[to]) seen[to] = true, vertices.push_back(to);
            return vertices;
        };
        auto verify = [&] {
            for (nidx_t check = 0; check < 20; ++check) {
                nidx_t vertex = nidx_t(rng() % n);
                auto vertices = component(vertex);
                long long sum = 0;
                for (nidx_t x : vertices) sum += value[x];
                CHECK(forest.component_size(vertex) == nidx_t(vertices.size()));
                CHECK(forest.fold(vertex) == sum);
                nidx_t other = nidx_t(rng() % n);
                CHECK(forest.connected(vertex, other) ==
                      (find(vertices.begin(), vertices.end(), other) != vertices.end()));
            }
        };

        for (nidx_t step = 0; step < 3000; ++step) {
            nidx_t operation = nidx_t(rng() % 4);
            if (operation == 0) {
                nidx_t vertex = nidx_t(rng() % n);
                value[vertex] = nidx_t(rng() % 201) - 100;
                forest.set(vertex, value[vertex]);
            } else if (operation <= 2) {
                nidx_t a = nidx_t(rng() % n), b = nidx_t(rng() % n);
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
                advance(it, nidx_t(rng() % edges.size()));
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

    nidx_t n = 5000;
    vector<long long> ones(n, 1);
    nett_forest<long long> chain(nall(ones));
    for (nidx_t vertex = 1; vertex < n; ++vertex) chain.link(vertex - 1, vertex);
    CHECK(chain.component_size(0) == n && chain.fold(n - 1) == n);
    for (nidx_t vertex = 1; vertex < n; vertex += 2) chain.cut(vertex - 1, vertex);
    CHECK(chain.component_size(0) == 1);
    for (nidx_t vertex = 2; vertex < n; vertex += 2)
        CHECK(chain.component_size(vertex) == 2);
}
