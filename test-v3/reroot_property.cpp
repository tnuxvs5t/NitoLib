#include "../src-v3/tree.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct weighted_edge { int to, weight; };
struct distance_state { long long count, distance; friend bool operator==(distance_state, distance_state) = default; };
struct distance_merge {
    distance_state id() const { return {0, 0}; }
    distance_state operator()(distance_state a, distance_state b) const {
        return {a.count + b.count, a.distance + b.distance};
    }
};

int main() {
    mt19937 random(0x7265726f6f74ULL);
    for (int trial = 0; trial < 1200; ++trial) {
        int n = 1 + int(random() % 35);
        vector<vector<weighted_edge>> adjacency(n);
        for (int vertex = 1; vertex < n; ++vertex) {
            int parent = int(random() % vertex), weight = 1 + int(random() % 20);
            adjacency[parent].push_back({vertex, weight});
            adjacency[vertex].push_back({parent, weight});
        }
        for (auto& edges : adjacency) shuffle(edges.begin(), edges.end(), random);
        auto graph = ngraph{nrange(n), [&](int vertex) -> auto& { return adjacency[vertex]; },
                            [](weighted_edge edge) { return edge.to; }};
        auto answer = nreroot(
            graph,
            [](int) { return distance_state{1, 0}; },
            [](distance_state state, int, weighted_edge edge) {
                state.distance += state.count * edge.weight;
                return state;
            }, distance_merge{});
        for (int source = 0; source < n; ++source) {
            vector<long long> distance(n, -1);
            vector<int> stack{source};
            distance[source] = 0;
            for (int at = 0; at < int(stack.size()); ++at) {
                int from = stack[at];
                for (auto edge : adjacency[from]) if (distance[edge.to] < 0) {
                    distance[edge.to] = distance[from] + edge.weight;
                    stack.push_back(edge.to);
                }
            }
            CHECK(answer[source] == distance_state(n, accumulate(distance.begin(), distance.end(), 0LL)));
        }
    }

    vector<vector<int>> adjacency{{2, 1}, {0, 3}, {0}, {1}};
    auto graph = ngraph{nrange(4), [&](int vertex) -> auto& { return adjacency[vertex]; }};
    auto base = [](int vertex) { return string("{") + char('a' + vertex) + '}'; };
    auto lift = [](string state, int from, int edge) {
        return "<" + to_string(from) + "->" + to_string(edge) + ':' + state + '>';
    };
    struct concat {
        string id() const { return {}; }
        string operator()(string left, const string& right) const { return left += right; }
    };
    auto answer = nreroot(graph, base, lift, concat{});
    auto message = [&](auto&& self, int from, int blocked) -> string {
        string state = base(from);
        for (int to : adjacency[from]) if (to != blocked)
            state += lift(self(self, to, from), to, from);
        return state;
    };
    for (int root = 0; root < 4; ++root) CHECK(answer[root] == message(message, root, -1));
}
