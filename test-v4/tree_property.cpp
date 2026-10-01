#include "../src-v4/tree.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x << '\n'; abort(); } } while (false)

int main() {
    mt19937 rng(91357);
    for (int trial = 0; trial < 1000; ++trial) {
        nidx_t n = 1 + nidx_t(rng() % 80), root = nidx_t(rng() % n);
        vector<vector<nidx_t>> adj(n);
        for (nidx_t v = 1; v < n; ++v) {
            nidx_t p = nidx_t(rng() % v);
            adj[v].push_back(p);
            adj[p].push_back(v);
        }

        auto next = [&](nidx_t v) -> const auto& { return adj[v]; };
        auto tree = nroot(ngraph{n, next}, array<nidx_t, 1>{root});
        auto hld = nhld(move(tree));

        vector<nidx_t> parent(n, -1), dep(n), queue{root};
        parent[root] = root;
        for (nidx_t i = 0; i < n; ++i)
            for (nidx_t u : adj[queue[i]])
                if (parent[u] < 0) parent[u] = queue[i], dep[u] = dep[queue[i]] + 1,
                    queue.push_back(u);
        CHECK(hld.par == parent && hld.dep == dep);

        vector<nidx_t> seen(n);
        for (nidx_t p = 0; p < n; ++p) {
            nidx_t v = hld.at[p];
            CHECK(hld.pos[v] == p && ++seen[v] == 1);
            auto [left, right] = hld.subtree(v);
            for (nidx_t u = 0; u < n; ++u) {
                nidx_t x = u;
                while (x != v && x != root) x = parent[x];
                CHECK((x == v) == (left <= hld.pos[u] && hld.pos[u] < right));
            }
        }

        for (int query = 0; query < 80; ++query) {
            nidx_t a = nidx_t(rng() % n), b = nidx_t(rng() % n);
            nidx_t x = a, y = b;
            while (x != y) {
                if (dep[x] >= dep[y]) x = parent[x];
                else y = parent[y];
            }
            CHECK(hld.lca(a, b) == x);
            vector<nidx_t> got;
            hld.visit_path(a, b, [&](npath_piece piece) {
                for (nidx_t i = piece.left; i < piece.right; ++i)
                    got.push_back(hld.at[piece.reverse ? piece.right - 1 - (i - piece.left) : i]);
            });
            vector<nidx_t> want;
            x = a; y = b;
            while (x != y) {
                if (dep[x] >= dep[y]) want.push_back(x), x = parent[x];
                else y = parent[y];
            }
            want.push_back(x);
            vector<nidx_t> suffix;
            x = a; y = b;
            while (x != y) {
                if (dep[x] < dep[y]) suffix.push_back(y), y = parent[y];
                else x = parent[x];
            }
            while (!suffix.empty()) want.push_back(suffix.back()), suffix.pop_back();
            CHECK(got == want);
        }
    }
    cout << "v4 tree: 1000 random trees, HLD/subtree/LCA/path passed\n";
}
