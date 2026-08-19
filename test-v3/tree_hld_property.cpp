#include "../src-v3/segment.hpp"
#include "../src-v3/tree.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

vector<nidx_t> brute_path(nidx_t a, nidx_t b, const vector<nidx_t>& parent, const vector<nidx_t>& depth) {
    vector<nidx_t> left, right;
    while (a != b) {
        if (depth[a] >= depth[b]) left.push_back(a), a = parent[a];
        else right.push_back(b), b = parent[b];
    }
    left.push_back(a);
    while (!right.empty()) left.push_back(right.back()), right.pop_back();
    return left;
}

int main() {
    mt19937 rng(0x71EE);
    for (nidx_t round = 0; round < 2500; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 100);
        vector<nidx_t> parent(n), depth(n), subtree(n, 1);
        vector<vector<nidx_t>> children(n), adjacency(n);
        for (nidx_t vertex = 1; vertex < n; ++vertex) {
            parent[vertex] = nidx_t(rng() % vertex);
            depth[vertex] = depth[parent[vertex]] + 1;
            children[parent[vertex]].push_back(vertex);
            adjacency[parent[vertex]].push_back(vertex);
            adjacency[vertex].push_back(parent[vertex]);
        }
        for (nidx_t vertex = n; --vertex > 0;) subtree[parent[vertex]] += subtree[vertex];

        auto direct = nhld(nrange(n), nrange(1),
                           [&](nidx_t vertex) -> auto& { return children[vertex]; });
        auto graph = ngraph{nrange(n), [&](nidx_t vertex) -> auto& { return adjacency[vertex]; }};
        auto rooted = nroot(graph, nrange(1));
        auto projected = nhld(rooted);

        vector<nidx_t> seen(n);
        auto order = direct.order();
        auto position = direct.positions();
        vector<nidx_t> low(n, n), high(n, -1), descendants(n);
        for (nidx_t vertex = 0; vertex < n; ++vertex) {
            for (nidx_t ancestor = vertex;; ancestor = parent[ancestor]) {
                low[ancestor] = min(low[ancestor], position(vertex));
                high[ancestor] = max(high[ancestor], position(vertex));
                ++descendants[ancestor];
                if (ancestor == parent[ancestor]) break;
            }
        }
        for (nidx_t i = 0; i < n; ++i) {
            CHECK(0 <= position(i) && position(i) < n);
            ++seen[position(i)];
            CHECK(order[position(i)] == i);
            CHECK(descendants[i] == subtree[i]);
            CHECK(low[i] == position(i) && high[i] == position(i) + subtree[i] - 1);
        }
        CHECK(count(seen.begin(), seen.end(), 1) == n);

        vector<string> label(n);
        for (nidx_t i = 0; i < n; ++i) label[i] = char('a' + rng() % 5);
        auto base = nmap(order, [&](nidx_t vertex) { return label[vertex]; });
        nseg segment(move(base), concat{});

        for (nidx_t query = 0; query < 100; ++query) {
            nidx_t a = nidx_t(rng() % n), b = nidx_t(rng() % n);
            auto expected_vertices = brute_path(a, b, parent, depth);
            nidx_t expected_lca = *min_element(expected_vertices.begin(), expected_vertices.end(),
                                            [&](nidx_t x, nidx_t y) { return depth[x] < depth[y]; });
            CHECK(direct.lca(a, b) == expected_lca);
            CHECK(projected.lca(a, b) == expected_lca);

            vector<nidx_t> got_vertices;
            string got;
            for (auto piece : direct.path(a, b)) {
                string part = segment.fold(piece.left, piece.right);
                if (piece.reverse) reverse(part.begin(), part.end());
                got += part;
                if (piece.reverse)
                    for (nidx_t i = piece.right; i-- > piece.left;) got_vertices.push_back(order[i]);
                else
                    for (nidx_t i = piece.left; i < piece.right; ++i) got_vertices.push_back(order[i]);
            }
            string expected;
            for (nidx_t vertex : expected_vertices) expected += label[vertex];
            CHECK(got_vertices == expected_vertices && got == expected);
        }
    }

    vector<string> keys{"root", "left", "right", "leaf"};
    unordered_map<string, nidx_t> id{{"root", 0}, {"left", 1}, {"right", 2}, {"leaf", 3}};
    vector<vector<string>> children{{"left", "right"}, {"leaf"}, {}, {}};
    vector<string> roots{"root"};
    auto named = nhld(ninvert(nall(keys)), nall(roots),
                      [&](const string& key) -> auto& { return children[id[key]]; });
    CHECK(named.lca(string("leaf"), string("right")) == "root");
    CHECK(named.positions()("leaf") >= 0);
}
