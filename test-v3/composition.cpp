#include "../src-v3/automata.hpp"
#include "../src-v3/discrete.hpp"
#include "../src-v3/dynamic_tree.hpp"
#include "../src-v3/flow.hpp"
#include "../src-v3/geom.hpp"
#include "../src-v3/graph_algo.hpp"
#include "../src-v3/graph_store.hpp"
#include "../src-v3/linear.hpp"
#include "../src-v3/link_cut.hpp"
#include "../src-v3/number.hpp"
#include "../src-v3/opt.hpp"
#include "../src-v3/poly.hpp"
#include "../src-v3/string.hpp"
#include "../src-v3/tree.hpp"
#include "../src-v3/wavelet.hpp"

struct composition_edge {
    nidx_t from, to;
};

int main() {
    auto require = [](bool condition) {
        if (!condition) abort();
    };
    vector<nidx_t> values{2, 3, 5, 7, 11, 13};

    auto doubled = nmap_values(nanchors(nall(values)), [](nidx_t value) { return 2 * value; });
    auto grid = nproduct(nrange(2), nrange(3));
    require(doubled[4] == 22 && grid.len() == 6 && grid[4] == pair(1, 1));
    auto descending = norder(nall(values), greater<>{});
    require(descending[0] == 13 && descending[5] == 2);
    require((nprefix(nall(values)) == vector<nidx_t>{0, 2, 5, 10, 17, 28, 41}));
    require((nsuffix(nall(values)) == vector<nidx_t>{41, 39, 36, 31, 24, 13, 0}));

    nfhq<nidx_t> sequence;
    nidx_t root = sequence.build(nall(values));
    auto [left, right] = sequence.split(root, 2);
    root = sequence.merge(right, left);
    require(sequence.sequence(root)[0] == 5 && sequence.sequence(root)[5] == 3);

    nseg<nidx_t> range_sum(nall(values));
    require(range_sum.fold(1, 5) == 26);
    nwavelet wave(nall(values));
    require(wave.kth(1, 6, 2) == 7);

    vector<composition_edge> edge_storage{{0, 1}, {0, 2}, {1, 3}, {1, 4}, {2, 5}};
    auto graph = nmake_csr(6, nall(edge_storage),
                           [](const auto& edge) { return edge.from; },
                           [](const auto& edge) { return edge.to; });
    require((nbfs(graph, 0) == vector<nidx_t>{0, 1, 1, 2, 2, 2}));

    array<nidx_t, 1> roots{0};
    auto rooted = nroot(graph.view(), nall(roots));
    auto hld = nhld(rooted);
    vector<nidx_t> path;
    auto order = hld.order();
    for (auto piece : hld.path(3, 5)) {
        for (nidx_t i = 0; i < piece.right - piece.left; ++i) {
            nidx_t position = piece.reverse ? piece.right - 1 - i : piece.left + i;
            path.push_back(order[position]);
        }
    }
    require((path == vector<nidx_t>{3, 1, 0, 2, 5}));

    nett_forest<nidx_t> forest(nall(values));
    forest.link(0, 1);
    forest.link(1, 2);
    require(forest.connected(0, 2) && forest.fold(0) == 10);
    forest.cut(1, 2);
    require(!forest.connected(0, 2) && forest.fold(0) == 5);

    nlct<nidx_t> paths(nall(values));
    paths.link(0, 1);
    paths.link(1, 2);
    require(paths.fold(0, 2) == 10);

    ndinic<nidx_t> flow(4);
    flow.add(0, 1, 2);
    flow.add(0, 2, 1);
    flow.add(1, 3, 2);
    flow.add(2, 3, 1);
    require(flow.flow(0, 3) == 3);

    vector<nidx_t> polynomial{1, 2, 3};
    auto square = nconvolution(nall(polynomial), nall(polynomial));
    require(square.size() == 5 && square[2].value == 10);
    string text = "ababa", pattern = "aba";
    require(nkmp(nall(text), nall(pattern)) == vector<nidx_t>({0, 2}));
    nac automaton;
    nidx_t terminal = automaton.add(nall(pattern));
    automaton.build();
    require(automaton.occurrences(nall(text))[terminal] == 2);

    require(nisprime(1000000007));
    nmatrix<nmodint<998244353>> matrix(2, 2);
    matrix(0, 0) = matrix(1, 1) = 2;
    matrix(0, 1) = matrix(1, 0) = 1;
    require(ndeterminant(matrix) == nmodint<998244353>(3));

    constexpr long long inf = (1LL << 60);
    nlichao<nline<long long>, nidx_t, long long> lines(-10, 11, inf);
    nidx_t line_root = lines.add(-1, {2, 3});
    require(lines.query(line_root, 4) == 11);

    vector<npoint<long long>> points{{0, 0}, {2, 0}, {2, 2}, {0, 2}, {1, 1}};
    auto hull = nconvex_hull(nall(points));
    require(hull.size() == 4 && npolygon_area2(nall(hull)) == 8);
}
