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
    for (int trial = 0; trial < 500; ++trial) {
        int n = 1 + int(random() % 24);
        vector<string> value(n);
        for (int i = 0; i < n; ++i) value[i] = char('a' + random() % 26);
        nlct forest(nall(value), concat{});
        vector<set<int>> adjacency(n);
        set<pair<int, int>> edges;

        auto path = [&](int source, int target) {
            vector<int> parent(n, -1), queue{source};
            parent[source] = source;
            for (int at = 0; at < int(queue.size()); ++at)
                for (int to : adjacency[queue[at]]) if (parent[to] < 0)
                    parent[to] = queue[at], queue.push_back(to);
            vector<int> result;
            if (parent[target] < 0) return result;
            for (int at = target;; at = parent[at]) {
                result.push_back(at);
                if (at == source) break;
            }
            reverse(result.begin(), result.end());
            return result;
        };

        for (int step = 0; step < 1200; ++step) {
            int operation = int(random() % 5);
            int a = int(random() % n), b = int(random() % n);
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
                    for (int vertex : route) expected += value[vertex];
                    check(forest.fold(a, b) == expected, "ordered path fold");
                    check(forest.path_size(a, b) == int(route.size()), "path size");
                    reverse(expected.begin(), expected.end());
                    check(forest.fold(b, a) == expected, "reverse path fold");
                }
            }
        }
    }

    int n = 10000;
    vector<long long> value(n, 1);
    nlct<long long> chain(nall(value));
    for (int i = 1; i < n; ++i) chain.link(i - 1, i);
    check(chain.fold(0, n - 1) == n, "deep chain fold");
    for (int i = 0; i < 20000; ++i) {
        int a = int(random() % n), b = int(random() % n);
        check(chain.path_size(a, b) == abs(a - b) + 1, "deep chain path size");
    }
}
