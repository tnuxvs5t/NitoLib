#include "../src-v4/flow.hpp"
#include "../src-v4/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct move_edge {
    nidx_t from, to, weight;
    unique_ptr<nidx_t> marker;

    move_edge(nidx_t a, nidx_t b, nidx_t w)
        : from(a), to(b), weight(w), marker(make_unique<nidx_t>(w)) {}
    move_edge(move_edge&&) = default;
    move_edge& operator=(move_edge&&) = default;
    move_edge(const move_edge&) = delete;
    move_edge& operator=(const move_edge&) = delete;
};

int main() {
    mt19937 rng(0xF104);

    {
        ndinic<nidx_t> flow(4);
        flow.add(0, 1, 2);
        flow.add(0, 2, 1);
        flow.add(1, 3, 2);
        flow.add(2, 3, 1);
        CHECK(flow.flow(0, 3, 1) == 1);
        CHECK(flow.flow(0, 3) == 2);
        CHECK(flow.flow(0, 3) == 0);
        CHECK(flow.cut(0) == vector<unsigned char>({1, 0, 0, 0}));
        CHECK(flow.flow(2, 2) == 0);
    }

    for (nidx_t round = 0; round < 2500; ++round) {
        nidx_t n = 2 + nidx_t(rng() % 7), sink = n - 1;
        ndinic<nidx_t> flow(n);
        struct original { nidx_t from, to, capacity, handle; };
        vector<original> edges;
        for (nidx_t from = 0; from < n; ++from)
            for (nidx_t to = 0; to < n; ++to)
                if (from != to && rng() % 5 == 0) {
                    nidx_t capacity = nidx_t(rng() % 10);
                    edges.push_back({from, to, capacity,
                                     flow.add(from, to, capacity)});
                }

        nidx_t expected = numeric_limits<nidx_t>::max();
        for (nidx_t mask = 0; mask < (nidx_t(1) << n); ++mask) {
            if (!(mask & 1) || mask & (nidx_t(1) << sink)) continue;
            nidx_t capacity = 0;
            for (auto edge : edges)
                if ((mask >> edge.from & 1) && !(mask >> edge.to & 1))
                    capacity += edge.capacity;
            expected = min(expected, capacity);
        }
        nidx_t got = flow.flow(0, sink);
        CHECK(got == expected);
        auto side = flow.cut(0);
        nidx_t cut = 0;
        for (auto edge : edges) {
            if (side[edge.from] && !side[edge.to]) cut += edge.capacity;
            nidx_t sent = edge.capacity - flow.edges[edge.handle].capacity;
            CHECK(0 <= sent && sent <= edge.capacity);
        }
        CHECK(cut == got && side[0] && !side[sink]);
    }

    for (nidx_t round = 0; round < 4000; ++round) {
        nidx_t left_size = nidx_t(rng() % 9), right_size = nidx_t(rng() % 9);
        vector<vector<nidx_t>> adj(left_size);
        for (nidx_t left = 0; left < left_size; ++left)
            for (nidx_t right = 0; right < right_size; ++right)
                if (rng() & 1) adj[left].push_back(right);

        auto matching = nhopcroft_karp(
            left_size, right_size,
            [&](nidx_t left) -> auto& { return adj[left]; });

        vector<nidx_t> best(nidx_t(1) << right_size, -1000);
        best[0] = 0;
        for (nidx_t left = 0; left < left_size; ++left) {
            auto next = best;
            for (nidx_t mask = 0; mask < nidx_t(best.size()); ++mask)
                for (nidx_t right : adj[left])
                    if (!(mask >> right & 1))
                        next[mask | (nidx_t(1) << right)] =
                            max(next[mask | (nidx_t(1) << right)], best[mask] + 1);
            best.swap(next);
        }
        CHECK(matching.size == *max_element(best.begin(), best.end()));
        vector<nidx_t> used(right_size, -1);
        for (nidx_t left = 0; left < left_size; ++left)
            if (matching.left[left] >= 0) {
                nidx_t right = matching.left[left];
                CHECK(find(adj[left].begin(), adj[left].end(), right) != adj[left].end());
                CHECK(used[right] < 0 && matching.right[right] == left);
                used[right] = left;
            }
    }

    {
        vector<move_edge> edges;
        edges.emplace_back(0, 1, 4);
        edges.emplace_back(1, 2, 2);
        edges.emplace_back(0, 2, 8);
        edges.emplace_back(3, 4, -3);
        edges.emplace_back(3, 3, 1);
        auto result = nkruskal(
            5, nall(edges),
            [](const move_edge& edge) { return edge.from; },
            [](const move_edge& edge) { return edge.to; },
            [](const move_edge& edge) { return edge.weight; });
        CHECK(result.weight == 3);
        CHECK(result.edges == vector<nidx_t>({3, 1, 0}));
    }

    for (nidx_t round = 0; round < 1000; ++round) {
        nidx_t n = 2 + nidx_t(rng() % 6);
        vector<tuple<nidx_t, nidx_t, nidx_t>> edges;
        for (nidx_t i = 0; i < 12; ++i) {
            nidx_t a = nidx_t(rng() % n), b = nidx_t(rng() % n);
            edges.emplace_back(a, b, nidx_t(rng() % 21) - 10);
        }
        auto result = nkruskal(
            n, nall(edges),
            [](const auto& edge) { return get<0>(edge); },
            [](const auto& edge) { return get<1>(edge); },
            [](const auto& edge) { return get<2>(edge); });

        ndsu original(n);
        for (auto edge : edges) original.merge(get<0>(edge), get<1>(edge));
        nidx_t components = 0;
        for (nidx_t vertex = 0; vertex < n; ++vertex)
            if (original.find(vertex) == vertex) ++components;

        nidx_t expected = numeric_limits<nidx_t>::max();
        nidx_t need = n - components;
        for (nidx_t mask = 0; mask < (nidx_t(1) << edges.size()); ++mask) {
            if (popcount(nuidx_t(mask)) != need) continue;
            ndsu selected(n);
            nidx_t weight = 0;
            bool forest = true;
            for (nidx_t i = 0; i < nidx_t(edges.size()); ++i)
                if (mask >> i & 1) {
                    if (selected.same(get<0>(edges[i]), get<1>(edges[i])))
                        forest = false;
                    else
                        selected.merge(get<0>(edges[i]), get<1>(edges[i]));
                    weight += get<2>(edges[i]);
                }
            if (!forest) continue;
            bool spans = true;
            for (nidx_t a = 0; a < n && spans; ++a)
                for (nidx_t b = a + 1; b < n; ++b)
                    spans &= original.same(a, b) == selected.same(a, b);
            if (spans) expected = min(expected, weight);
        }
        CHECK(result.weight == expected && nidx_t(result.edges.size()) == need);
    }

    cout << "v4 flow: incremental Dinic, matching and spanning forests passed\n";
}
