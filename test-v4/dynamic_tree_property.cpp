#include "../src-v4/dynamic_tree.hpp"
#include "../src-v4/link_cut.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

template <class F>
vector<nidx_t> route(nidx_t n, const vector<set<nidx_t>>& graph,
                     nidx_t source, nidx_t target, F&& visit) {
    vector<nidx_t> parent(n, -1), queue{source};
    parent[source] = source;
    for (nidx_t at = 0; at < nidx_t(queue.size()); ++at)
        for (nidx_t to : graph[queue[at]])
            if (parent[to] < 0) parent[to] = queue[at], queue.push_back(to);
    if (parent[target] < 0) return {};
    vector<nidx_t> path;
    for (nidx_t at = target;; at = parent[at]) {
        path.push_back(at);
        if (at == source) break;
    }
    ranges::reverse(path);
    invoke(visit, path);
    return path;
}

int main() {
    mt19937 rng(0xE771);
    for (nidx_t trial = 0; trial < 260; ++trial) {
        nidx_t n = 1 + nidx_t(rng() % 28);
        vector<long long> values(n);
        for (auto& value : values) value = nidx_t(rng() % 101) - 50;
        nett_forest<long long> ett(nall(values));
        vector<set<nidx_t>> graph(n);
        set<pair<nidx_t, nidx_t>> edges;

        auto verify_ett = [&] {
            vector<unsigned char> seen(n);
            for (nidx_t source = 0; source < n; ++source) if (!seen[source]) {
                vector<nidx_t> part;
                route(n, graph, source, source, [&](const auto&) {});
                vector<nidx_t> queue{source};
                seen[source] = true;
                for (nidx_t at = 0; at < nidx_t(queue.size()); ++at)
                    for (nidx_t to : graph[queue[at]])
                        if (!seen[to]) seen[to] = true, queue.push_back(to);
                long long sum = 0;
                for (nidx_t vertex : queue) sum += values[vertex];
                CHECK(ett.component_size(source) == nidx_t(queue.size()));
                CHECK(ett.fold(source) == sum);
                for (nidx_t vertex : queue) CHECK(ett.connected(source, vertex));
            }
        };

        for (nidx_t step = 0; step < 1800; ++step) {
            nidx_t operation = nidx_t(rng() % 4);
            if (operation == 0) {
                nidx_t vertex = nidx_t(rng() % n);
                values[vertex] = nidx_t(rng() % 201) - 100;
                ett.set(vertex, values[vertex]);
            } else if (operation <= 2) {
                nidx_t a = nidx_t(rng() % n), b = nidx_t(rng() % n);
                if (a == b || ett.connected(a, b)) continue;
                ett.link(a, b);
                graph[a].insert(b);
                graph[b].insert(a);
                edges.emplace(min(a, b), max(a, b));
            } else if (!edges.empty()) {
                auto it = edges.begin();
                advance(it, nidx_t(rng() % edges.size()));
                auto [a, b] = *it;
                ett.cut(a, b);
                graph[a].erase(b);
                graph[b].erase(a);
                edges.erase(it);
            }
            if (step % 41 == 0) verify_ett();
        }
        verify_ett();
    }

    for (nidx_t trial = 0; trial < 180; ++trial) {
        nidx_t n = 1 + nidx_t(rng() % 22);
        vector<string> values(n);
        for (auto& value : values) value = string(1, char('a' + rng() % 26));
        nlct forest(nall(values), concat{});
        vector<set<nidx_t>> graph(n);
        set<pair<nidx_t, nidx_t>> edges;
        for (nidx_t step = 0; step < 900; ++step) {
            nidx_t a = nidx_t(rng() % n), b = nidx_t(rng() % n);
            vector<nidx_t> path = route(n, graph, a, b, [&](const auto&) {});
            nidx_t operation = nidx_t(rng() % 5);
            if (operation == 0 && a != b && path.empty()) {
                forest.link(a, b);
                graph[a].insert(b); graph[b].insert(a); edges.emplace(min(a, b), max(a, b));
            } else if (operation == 1 && !edges.empty()) {
                auto it = edges.begin();
                advance(it, nidx_t(rng() % edges.size()));
                auto [x, y] = *it;
                forest.cut(x, y);
                graph[x].erase(y); graph[y].erase(x); edges.erase(it);
            } else if (operation == 2) {
                values[a] = string(1, char('a' + rng() % 26));
                forest.set(a, values[a]);
                CHECK(forest.get(a) == values[a]);
            } else {
                CHECK(forest.connected(a, b) == !path.empty());
                if (!path.empty()) {
                    string expected;
                    for (nidx_t vertex : path) expected += values[vertex];
                    CHECK(forest.fold(a, b) == expected);
                    CHECK(forest.path_size(a, b) == nidx_t(path.size()));
                }
            }
        }
    }

    nidx_t n = 5000;
    vector<long long> ones(n, 1);
    nett_forest<long long> chain(nall(ones));
    for (nidx_t i = 1; i < n; ++i) chain.link(i - 1, i);
    CHECK(chain.component_size(0) == n && chain.fold(n - 1) == n);
    for (nidx_t i = 1; i < n; i += 2) chain.cut(i - 1, i);
    CHECK(chain.component_size(0) == 1);

    vector<long long> unit(7000, 1);
    nlct<long long> long_chain(nall(unit));
    for (nidx_t i = 1; i < nidx_t(unit.size()); ++i) long_chain.link(i - 1, i);
    CHECK(long_chain.fold(0, nidx_t(unit.size()) - 1) == nidx_t(unit.size()));
    cout << "v4 dynamic trees: Euler-tour components and ordered link-cut paths passed\n";
}
