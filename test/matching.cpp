#include "common.hpp"

static int brute_matching(const nvector<nvector<int>>& adjacency, int left = 0, unsigned used = 0) {
    if (left == adjacency.len())
        return 0;
    int result = brute_matching(adjacency, left + 1, used);
    for (int i = 0; i < adjacency[left].len(); ++i) {
        int right = adjacency[left][i];
        if (!(used >> right & 1U))
            nchmax(result, 1 + brute_matching(adjacency, left + 1, used | (1U << right)));
    }
    return result;
}

int main() {
    nvector<nvector<int>> sample{{0, 1}, {0}, {1, 2}, {2}};
    auto graph = ngraph_view(sample.len(), [&](int left) -> const nvector<int>& { return sample[left]; });
    auto matching = nhopcroft_karp(graph, 3);
    ntest(matching.size == 3);

    mt19937 random(0x44a7c4U);
    for (int left_vertices = 0; left_vertices <= 8; ++left_vertices)
        for (int right_vertices = 0; right_vertices <= 8; ++right_vertices)
            for (int repeat = 0; repeat < 120; ++repeat) {
                nvector<nvector<int>> adjacency(left_vertices);
                for (int left = 0; left < left_vertices; ++left)
                    for (int right = 0; right < right_vertices; ++right)
                        if (random() % 4 == 0)
                            adjacency[left].push(right);
                auto implicit = ngraph_view(left_vertices, [&](int left) -> const nvector<int>& {
                    return adjacency[left];
                });
                auto got = nhopcroft_karp(implicit, right_vertices);
                ntest(got.size == brute_matching(adjacency));
                int count = 0;
                for (int left = 0; left < left_vertices; ++left)
                    if (got.left[left] != npos) {
                        ++count;
                        ntest(got.right[got.left[left]] == left);
                        ntest(nfind(adjacency[left], got.left[left]) != npos);
                    }
                ntest(count == got.size);
            }

    nbimatch stateful(4, 3);
    nvector<pair<int, int>> edges{{0, 0}, {0, 1}, {1, 0}, {2, 1}, {2, 2}, {3, 2}};
    nfor(edge, edges)
        stateful.add(edge.first, edge.second);
    int size = stateful.solve();
    ntest(size == 3 && stateful.pairs().len() == 3);
    auto cover = stateful.mincover();
    ntest(cover.l.len() + cover.r.len() == size);
    nfor(edge, edges) {
        bool covered = nfind(cover.l, edge.first) != npos || nfind(cover.r, edge.second) != npos;
        ntest(covered);
    }
}
