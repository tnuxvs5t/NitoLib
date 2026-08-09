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
    int from, to;
};

int main() {
    auto require = [](bool condition) {
        if (!condition) abort();
    };
    vector<int> values{2, 3, 5, 7, 11, 13};

    auto doubled = nmap_values(nfunc_bind(nall(values)), [](int value) { return 2 * value; });
    auto grid = nproduct(nrange(2), nrange(3));
    require(doubled[4] == 22 && grid.len() == 6 && grid[4] == pair(1, 1));
    auto descending = norder(nall(values), greater<>{});
    require(descending[0] == 13 && descending[5] == 2);
    require((nprefix(nall(values)) == vector<int>{0, 2, 5, 10, 17, 28, 41}));
    require((nsuffix(nall(values)) == vector<int>{41, 39, 36, 31, 24, 13, 0}));

    nfhq<int> sequence;
    int root = sequence.build(nall(values));
    auto [left, right] = sequence.split(root, 2);
    root = sequence.merge(right, left);
    require(sequence.sequence(root)[0] == 5 && sequence.sequence(root)[5] == 3);

    nseg<int> range_sum(nall(values));
    require(range_sum.fold(1, 5) == 26);
    nwavelet wave(nall(values));
    require(wave.kth(1, 6, 2) == 7);

    vector<composition_edge> edge_storage{{0, 1}, {0, 2}, {1, 3}, {1, 4}, {2, 5}};
    auto graph = nmake_csr(6, nall(edge_storage),
                           [](const auto& edge) { return edge.from; },
                           [](const auto& edge) { return edge.to; });
    require((nbfs(graph, 0) == vector<int>{0, 1, 1, 2, 2, 2}));

    array<int, 1> roots{0};
    auto rooted = nroot(graph.view(), nall(roots));
    auto hld = nhld(rooted);
    vector<int> path;
    auto order = hld.order();
    for (auto piece : hld.path(3, 5)) {
        for (int i = 0; i < piece.right - piece.left; ++i) {
            int position = piece.reverse ? piece.right - 1 - i : piece.left + i;
            path.push_back(order[position]);
        }
    }
    require((path == vector<int>{3, 1, 0, 2, 5}));

    nett_forest<int> forest(nall(values));
    forest.link(0, 1);
    forest.link(1, 2);
    require(forest.connected(0, 2) && forest.fold(0) == 10);
    forest.cut(1, 2);
    require(!forest.connected(0, 2) && forest.fold(0) == 5);

    nlct<int> paths(nall(values));
    paths.link(0, 1);
    paths.link(1, 2);
    require(paths.fold(0, 2) == 10);

    ndinic<int> flow(4);
    flow.add(0, 1, 2);
    flow.add(0, 2, 1);
    flow.add(1, 3, 2);
    flow.add(2, 3, 1);
    require(flow.flow(0, 3) == 3);

    vector<int> polynomial{1, 2, 3};
    auto square = nconvolution(nall(polynomial), nall(polynomial));
    require(square.size() == 5 && square[2].value == 10);
    string text = "ababa", pattern = "aba";
    require(nkmp(nall(text), nall(pattern)) == vector<int>({0, 2}));
    nac automaton;
    int terminal = automaton.add(nall(pattern));
    automaton.build();
    require(automaton.occurrences(nall(text))[terminal] == 2);

    require(nisprime(1000000007));
    nmatrix<nmodint<998244353>> matrix(2, 2);
    matrix(0, 0) = matrix(1, 1) = 2;
    matrix(0, 1) = matrix(1, 0) = 1;
    require(ndeterminant(matrix) == nmodint<998244353>(3));

    constexpr long long inf = (1LL << 60);
    nlichao<nline<long long>, int, long long> lines(-10, 11, inf);
    int line_root = lines.add(-1, {2, 3});
    require(lines.query(line_root, 4) == 11);

    vector<npoint<long long>> points{{0, 0}, {2, 0}, {2, 2}, {0, 2}, {1, 1}};
    auto hull = nconvex_hull(nall(points));
    require(hull.size() == 4 && npolygon_area2(nall(hull)) == 8);
}
