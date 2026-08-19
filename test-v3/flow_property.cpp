#include "../src-v3/flow.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct weighted_edge { nidx_t from, to, weight; };

int main() {
    mt19937 rng(0xF10);
    for (nidx_t round = 0; round < 3000; ++round) {
        nidx_t n = 2 + nidx_t(rng() % 7), source = 0, sink = n - 1;
        ndinic<nidx_t> flow(n);
        struct original { nidx_t from, to, capacity, handle; };
        vector<original> edges;
        for (nidx_t from = 0; from < n; ++from)
            for (nidx_t to = 0; to < n; ++to)
                if (from != to && rng() % 5 == 0) {
                    nidx_t capacity = nidx_t(rng() % 10);
                    edges.push_back({from, to, capacity, flow.add(from, to, capacity)});
                }
        nidx_t expected = INT_MAX;
        for (nidx_t mask = 0; mask < (1 << n); ++mask) {
            if (!(mask & 1) || mask & (1 << sink)) continue;
            nidx_t capacity = 0;
            for (auto edge : edges)
                if ((mask >> edge.from & 1) && !(mask >> edge.to & 1)) capacity += edge.capacity;
            expected = min(expected, capacity);
        }
        nidx_t got = flow.flow(source, sink);
        CHECK(got == expected);
        auto side = flow.cut(source);
        nidx_t capacity = 0;
        for (auto edge : edges) {
            if (side[edge.from] && !side[edge.to]) capacity += edge.capacity;
            nidx_t sent = edge.capacity - flow.edges[edge.handle].capacity;
            CHECK(0 <= sent && sent <= edge.capacity);
        }
        CHECK(capacity == got && side[source] && !side[sink]);
    }

    for (nidx_t round = 0; round < 5000; ++round) {
        nidx_t left_size = nidx_t(rng() % 9), right_size = nidx_t(rng() % 9);
        vector<vector<nidx_t>> adjacency(left_size);
        for (nidx_t left = 0; left < left_size; ++left)
            for (nidx_t right = 0; right < right_size; ++right)
                if (rng() & 1) adjacency[left].push_back(right);
        auto matching = nhopcroft_karp(left_size, right_size,
                                       [&](nidx_t left) -> auto& { return adjacency[left]; });
        vector<nidx_t> dp(1 << right_size, -1000);
        dp[0] = 0;
        for (nidx_t left = 0; left < left_size; ++left) {
            auto next = dp;
            for (nidx_t mask = 0; mask < nidx_t(dp.size()); ++mask)
                for (nidx_t right : adjacency[left])
                    if (!(mask >> right & 1))
                        next[mask | (1 << right)] = max(next[mask | (1 << right)], dp[mask] + 1);
            dp.swap(next);
        }
        CHECK(matching.size == *max_element(dp.begin(), dp.end()));
        vector<nidx_t> used(right_size, -1);
        for (nidx_t left = 0; left < left_size; ++left)
            if (matching.left[left] >= 0) {
                nidx_t right = matching.left[left];
                CHECK(find(adjacency[left].begin(), adjacency[left].end(), right) != adjacency[left].end());
                CHECK(used[right] < 0 && matching.right[right] == left);
                used[right] = left;
            }
    }

    for (nidx_t round = 0; round < 800; ++round) {
        nidx_t n = 2 + nidx_t(rng() % 6);
        vector<weighted_edge> edges;
        for (nidx_t vertex = 1; vertex < n; ++vertex)
            edges.push_back({vertex - 1, vertex, nidx_t(rng() % 21) - 10});
        while (edges.size() < 12) {
            nidx_t a = nidx_t(rng() % n), b = nidx_t(rng() % n);
            if (a != b) edges.push_back({a, b, nidx_t(rng() % 21) - 10});
        }
        auto result = nkruskal(n, nall(edges), [](auto edge) { return edge.from; },
                               [](auto edge) { return edge.to; },
                               [](auto edge) { return edge.weight; });
        nidx_t expected = INT_MAX;
        for (nidx_t mask = 0; mask < (1 << nidx_t(edges.size())); ++mask) {
            if (popcount(unsigned(mask)) != n - 1) continue;
            ndsu components(n);
            nidx_t weight = 0;
            for (nidx_t i = 0; i < nidx_t(edges.size()); ++i)
                if (mask >> i & 1) components.merge(edges[i].from, edges[i].to), weight += edges[i].weight;
            bool connected = true;
            for (nidx_t vertex = 1; vertex < n; ++vertex) connected &= components.same(0, vertex);
            if (connected) expected = min(expected, weight);
        }
        CHECK(result.weight == expected && nidx_t(result.edges.size()) == n - 1);
    }
}
