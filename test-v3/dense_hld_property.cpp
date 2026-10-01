#include "../src-v3/tree.hpp"
#include <sys/resource.h>

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    rlimit stack{};
    CHECK(getrlimit(RLIMIT_STACK, &stack) == 0);
    stack.rlim_cur = min(stack.rlim_max, rlim_t(8 * 1024 * 1024));
    CHECK(setrlimit(RLIMIT_STACK, &stack) == 0);
    const nidx_t n = 300000;
    auto chain = [=](nidx_t v) {
        array<nidx_t, 2> neighbors{v - 1, v + 1};
        return move(neighbors) | views::filter([=](nidx_t u) { return 0 <= u && u < n; });
    };
    auto deep = nhld(n, 0, chain);
    CHECK(deep.lca(n - 1, n / 2) == n / 2);
    CHECK(deep.subtree_value[0] == n);
    CHECK(deep.position_value[n - 1] == n - 1);
    auto distance = nbfs(ngraph{n, chain}, 0);
    CHECK(distance.back() == n - 1);

    mt19937 rng(77913);
    for (int round = 0; round < 400; ++round) {
        nidx_t count = 1 + nidx_t(rng() % 80), root = nidx_t(rng() % count);
        vector<vector<nidx_t>> adjacency(count);
        for (nidx_t v = 1; v < count; ++v) {
            nidx_t p = nidx_t(rng() % v);
            adjacency[v].push_back(p);
            adjacency[p].push_back(v);
        }
        auto hld = nhld(count, root, [&](nidx_t v) -> auto& { return adjacency[v]; });
        auto graph = ngraph{count, [&](nidx_t v) -> auto& { return adjacency[v]; }};
        auto rooted = nroot(graph, array{root});
        CHECK(rooted.parent_position == hld.parent_position);
        for (int q = 0; q < 30; ++q) {
            nidx_t a = nidx_t(rng() % count), b = nidx_t(rng() % count);
            // Independent BFS from a reconstructs the unique a->b path.
            vector<nidx_t> parent(count, -1), queue{a};
            parent[a] = a;
            for (size_t i = 0; i < queue.size(); ++i)
                for (auto to : adjacency[queue[i]])
                    if (parent[to] < 0) parent[to] = queue[i], queue.push_back(to);
            vector<nidx_t> expected{b};
            while (expected.back() != a) expected.push_back(parent[expected.back()]);
            reverse(expected.begin(), expected.end());
            vector<nidx_t> got;
            hld.visit_path(a, b, [&](npath_piece piece) {
                for (nidx_t i = 0; i < piece.right - piece.left; ++i)
                    got.push_back(hld.vertex_at_position[piece.reverse ? piece.right - i - 1 : piece.left + i]);
            });
            CHECK(got == expected);
        }
    }
}
