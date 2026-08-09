#include "../src-v3/flow.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct weighted_edge { int from, to, weight; };

int main() {
    mt19937 rng(0xF10);
    for (int round = 0; round < 3000; ++round) {
        int n = 2 + int(rng() % 7), source = 0, sink = n - 1;
        ndinic<int> flow(n);
        struct original { int from, to, capacity, handle; };
        vector<original> edges;
        for (int from = 0; from < n; ++from)
            for (int to = 0; to < n; ++to)
                if (from != to && rng() % 5 == 0) {
                    int capacity = int(rng() % 10);
                    edges.push_back({from, to, capacity, flow.add(from, to, capacity)});
                }
        int expected = INT_MAX;
        for (int mask = 0; mask < (1 << n); ++mask) {
            if (!(mask & 1) || mask & (1 << sink)) continue;
            int capacity = 0;
            for (auto edge : edges)
                if ((mask >> edge.from & 1) && !(mask >> edge.to & 1)) capacity += edge.capacity;
            expected = min(expected, capacity);
        }
        int got = flow.flow(source, sink);
        CHECK(got == expected);
        auto side = flow.cut(source);
        int capacity = 0;
        for (auto edge : edges) {
            if (side[edge.from] && !side[edge.to]) capacity += edge.capacity;
            int sent = edge.capacity - flow.edges[edge.handle].capacity;
            CHECK(0 <= sent && sent <= edge.capacity);
        }
        CHECK(capacity == got && side[source] && !side[sink]);
    }

    for (int round = 0; round < 5000; ++round) {
        int left_size = int(rng() % 9), right_size = int(rng() % 9);
        vector<vector<int>> adjacency(left_size);
        for (int left = 0; left < left_size; ++left)
            for (int right = 0; right < right_size; ++right)
                if (rng() & 1) adjacency[left].push_back(right);
        auto matching = nhopcroft_karp(left_size, right_size,
                                       [&](int left) -> auto& { return adjacency[left]; });
        vector<int> dp(1 << right_size, -1000);
        dp[0] = 0;
        for (int left = 0; left < left_size; ++left) {
            auto next = dp;
            for (int mask = 0; mask < int(dp.size()); ++mask)
                for (int right : adjacency[left])
                    if (!(mask >> right & 1))
                        next[mask | (1 << right)] = max(next[mask | (1 << right)], dp[mask] + 1);
            dp.swap(next);
        }
        CHECK(matching.size == *max_element(dp.begin(), dp.end()));
        vector<int> used(right_size, -1);
        for (int left = 0; left < left_size; ++left)
            if (matching.left[left] >= 0) {
                int right = matching.left[left];
                CHECK(find(adjacency[left].begin(), adjacency[left].end(), right) != adjacency[left].end());
                CHECK(used[right] < 0 && matching.right[right] == left);
                used[right] = left;
            }
    }

    for (int round = 0; round < 800; ++round) {
        int n = 2 + int(rng() % 6);
        vector<weighted_edge> edges;
        for (int vertex = 1; vertex < n; ++vertex)
            edges.push_back({vertex - 1, vertex, int(rng() % 21) - 10});
        while (edges.size() < 12) {
            int a = int(rng() % n), b = int(rng() % n);
            if (a != b) edges.push_back({a, b, int(rng() % 21) - 10});
        }
        auto result = nkruskal(n, nall(edges), [](auto edge) { return edge.from; },
                               [](auto edge) { return edge.to; },
                               [](auto edge) { return edge.weight; });
        int expected = INT_MAX;
        for (int mask = 0; mask < (1 << int(edges.size())); ++mask) {
            if (popcount(unsigned(mask)) != n - 1) continue;
            ndsu components(n);
            int weight = 0;
            for (int i = 0; i < int(edges.size()); ++i)
                if (mask >> i & 1) components.merge(edges[i].from, edges[i].to), weight += edges[i].weight;
            bool connected = true;
            for (int vertex = 1; vertex < n; ++vertex) connected &= components.same(0, vertex);
            if (connected) expected = min(expected, weight);
        }
        CHECK(result.weight == expected && int(result.edges.size()) == n - 1);
    }
}
