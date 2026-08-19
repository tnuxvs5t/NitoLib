#include "../src-v3/link_cut.hpp"

struct concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

void check(bool condition, const char* message) {
    if (!condition) throw runtime_error(message);
}

int main() {
    mt19937 random(0x1c72026U);
    for (nidx_t trial = 0; trial < 500; ++trial) {
        nidx_t n = 1 + nidx_t(random() % 24);
        vector<string> value(n);
        for (nidx_t i = 0; i < n; ++i) value[i] = char('a' + random() % 26);
        nlct forest(nall(value), concat{});
        vector<set<nidx_t>> adjacency(n);
        set<pair<nidx_t, nidx_t>> edges;

        auto path = [&](nidx_t source, nidx_t target) {
            vector<nidx_t> parent(n, -1), queue{source};
            parent[source] = source;
            for (nidx_t at = 0; at < nidx_t(queue.size()); ++at)
                for (nidx_t to : adjacency[queue[at]]) if (parent[to] < 0)
                    parent[to] = queue[at], queue.push_back(to);
            vector<nidx_t> result;
            if (parent[target] < 0) return result;
            for (nidx_t at = target;; at = parent[at]) {
                result.push_back(at);
                if (at == source) break;
            }
            reverse(result.begin(), result.end());
            return result;
        };

        for (nidx_t step = 0; step < 1200; ++step) {
            nidx_t operation = nidx_t(random() % 5);
            nidx_t a = nidx_t(random() % n), b = nidx_t(random() % n);
            auto route = path(a, b);
            if (operation == 0 && a != b && route.empty()) {
                forest.link(a, b);
                adjacency[a].insert(b);
                adjacency[b].insert(a);
                edges.insert(minmax(a, b));
            } else if (operation == 1 && !edges.empty()) {
                auto it = edges.begin();
                advance(it, random() % edges.size());
                auto [x, y] = *it;
                forest.cut(x, y);
                adjacency[x].erase(y);
                adjacency[y].erase(x);
                edges.erase(it);
            } else if (operation == 2) {
                value[a] = string(1, char('a' + random() % 26));
                forest.set(a, value[a]);
                check(forest.get(a) == value[a], "get after set");
            } else {
                check(forest.connected(a, b) == !route.empty(), "connectivity");
                if (!route.empty()) {
                    string expected;
                    for (nidx_t vertex : route) expected += value[vertex];
                    check(forest.fold(a, b) == expected, "ordered path fold");
                    check(forest.path_size(a, b) == nidx_t(route.size()), "path size");
                    reverse(expected.begin(), expected.end());
                    check(forest.fold(b, a) == expected, "reverse path fold");
                }
            }
        }
    }

    nidx_t n = 10000;
    vector<long long> value(n, 1);
    nlct<long long> chain(nall(value));
    for (nidx_t i = 1; i < n; ++i) chain.link(i - 1, i);
    check(chain.fold(0, n - 1) == n, "deep chain fold");
    for (nidx_t i = 0; i < 20000; ++i) {
        nidx_t a = nidx_t(random() % n), b = nidx_t(random() % n);
        check(chain.path_size(a, b) == abs(a - b) + 1, "deep chain path size");
    }
}
