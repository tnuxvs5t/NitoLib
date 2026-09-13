#include "../src-v3/tree.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    mt19937 rng(52717);
    for (nidx_t trial = 0; trial < 1500; ++trial) {
        nidx_t n = trial ? nidx_t(rng() % 70) : 3;
        vector<vector<nidx_t>> children(n);
        vector<nidx_t> roots, parent(n, -1);
        for (nidx_t v = 0; v < n; ++v) {
            if (trial && v && rng() % 3) {
                parent[v] = nidx_t(rng() % v);
                children[parent[v]].push_back(v);
            } else if (trial ? bool(rng() & 1) : v == 2) roots.push_back(v);
        }
        // Queue oracle deliberately differs from the implementation's DFS.
        vector<nidx_t> depth(n, -1), subtree(n), queue = roots;
        for (nidx_t root : roots) depth[root] = 0;
        for (nidx_t i = 0; i < nidx_t(queue.size()); ++i)
            for (nidx_t child : children[queue[i]]) {
                depth[child] = depth[queue[i]] + 1;
                queue.push_back(child);
            }
        for (nidx_t v : queue)
            for (nidx_t u = v; u >= 0; u = parent[u]) ++subtree[u];
        vector<string> keys(n);
        for (nidx_t i = 0; i < n; ++i) keys[i] = "v" + to_string(i);
        auto vertices = ninvert(nall(keys));
        auto root_keys = nproject(nall(roots), [&](nidx_t u) { return keys[u]; });
        auto next_keys = [&](const string& key) {
            nidx_t u = vertices.inverse(key);
            return nproject(nall(children[u]), [&](nidx_t v) { return keys[v]; });
        };
        auto graph = ngraph{nall(vertices), next_keys};
        auto rooted = nroot(graph, root_keys);
        auto copy = nhld(rooted);
        auto moved = nhld(nroot(graph, root_keys));
        auto direct = nhld(nall(vertices), root_keys, next_keys);
        auto verify = [&](const auto& hld) {
            CHECK(hld.len() == n && hld.order().len() == nidx_t(queue.size()));
            vector<nidx_t> seen(n);
            for (nidx_t i = 0; i < hld.order().len(); ++i) {
                nidx_t v = vertices.inverse(hld.order()[i]);
                CHECK(++seen[v] == 1 && depth[v] >= 0 && hld.positions()(keys[v]) == i);
            }
            for (nidx_t v = 0; v < n; ++v) {
                CHECK(hld.depth_value[v] == depth[v] && hld.subtree_value[v] == subtree[v]);
                if (depth[v] < 0) {
                    CHECK(!seen[v] && hld.position_value[v] == -1 && hld.head_position[v] == -1);
                    CHECK(hld.parent_position[v] == -1 && hld.heavy_position[v] == -1);
                } else {
                    CHECK(seen[v] == 1 && hld.lca(keys[v], keys[v]) == keys[v]);
                    nidx_t count = 0;
                    hld.visit_path(keys[v], keys[v], [&](npath_piece piece) {
                        count += piece.right - piece.left;
                    });
                    CHECK(count == 1);
                }
            }
        };
        verify(copy); verify(moved); verify(direct);
    }
}
