// Deliberately no view.hpp, func.hpp or hash.hpp, including transitively.
#include "../src-v3/sequence.hpp"
#include "../src-v3/ds.hpp"
#include "../src-v3/segment.hpp"
#include "../src-v3/string.hpp"
#include "../src-v3/automata.hpp"
#include "../src-v3/wavelet.hpp"
#include "../src-v3/graph_algo.hpp"
#include "../src-v3/geom.hpp"
#include "../src-v3/linear.hpp"
#include "../src-v3/poly.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct noncopy_source {
    vector<long long> data;
    noncopy_source() = default;
    noncopy_source(const noncopy_source&) = delete;
    size_t size() const { return data.size(); }
    const long long& operator[](nidx_t i) const { return data[i]; }
};

int main() {
    mt19937 rng(54321);
    for (int round = 0; round < 300; ++round) {
        noncopy_source source;
        source.data.resize(1 + rng() % 60);
        for (auto& value : source.data) value = rng() % 100;
        nidx_t n = nlen(source);
        nfenwick bit(source);
        nseg seg(source, nadd<long long>{});
        nlazy_addsum<long long> lazy(source);
        nwavelet wave(source);
        nsparse_table sparse(source, nmin<long long>{});
        auto plan = nargsort(source);
        CHECK(is_sorted(plan.begin(), plan.end(), [&](auto a, auto b) { return source[a] < source[b]; }));
        for (int q = 0; q < 30; ++q) {
            nidx_t l = nidx_t(rng() % n), r = nidx_t(rng() % n);
            if (l > r) swap(l, r);
            ++r;
            vector<long long> sorted(source.data.begin() + l, source.data.begin() + r);
            sort(sorted.begin(), sorted.end());
            long long expected = accumulate(sorted.begin(), sorted.end(), 0LL);
            CHECK(bit.fold(l, r) == expected && seg.fold(l, r) == expected);
            CHECK(lazy.fold(l, r) == expected && sparse.fold(l, r) == sorted[0]);
            CHECK(wave.kth(l, r, (r - l) / 2) == sorted[(r - l) / 2]);
        }
        lazy.apply(0, n, 7);
        auto copied = lazy; // Must copy pending tags, not call a source constructor.
        copied.apply(0, n, 3);
        CHECK(copied.fold() == lazy.fold() + 3 * n);
        CHECK(lazy.fold() == seg.fold() + 7 * n);
        CHECK(nz(source) == nz(source.data));
    }
    array values{3, 1, 1, 4};
    const vector<nidx_t> positions{1, 1, 0};
    auto selected = ngather(span{values}, span{positions});
    selected[0] = 9;
    CHECK(selected[1] == 9 && values[1] == 9);
    CHECK((nrun_bounds(array{1, 1, 3, 3, 3, 1}) == vector<pair<nidx_t, nidx_t>>{{0, 2}, {2, 5}, {5, 6}}));
    string text = "ababa";
    CHECK((nkmp(text, string_view("aba")) == vector<nidx_t>{0, 2}));
    nac<> automaton;
    auto terminal = automaton.add(string_view("aba"));
    automaton.build();
    CHECK(automaton.occurrences(text)[terminal] == 2);
    array<long long, 2> polynomial{1, 2};
    auto convolution = nconvolution(polynomial, polynomial);
    CHECK(convolution[0] == 1 && convolution[1] == 4 && convolution[2] == 4);
    nmatrix<long long> matrix(2, 2);
    static_assert(is_same_v<decltype(matrix[0]), span<long long>>);
    matrix[0][1] = 7;
    CHECK(matrix(0, 1) == 7);
    array triangle{npoint<long long>{0, 0}, npoint<long long>{2, 0}, npoint<long long>{0, 3}};
    CHECK(npolygon_area2(triangle) == 6 && nconvex_hull(triangle).size() == 3);
    vector<vector<nidx_t>> adjacency{{1}, {0, 2}, {1}, {}};
    auto graph = ngraph{4, [&](nidx_t v) -> auto& { return adjacency[v]; }};
    CHECK((nbfs(graph, 0) == vector<nidx_t>{0, 1, 2, -1}));
    CHECK((nbfs_many(graph, array{0, 2}) == vector<nidx_t>{0, 1, 0, -1}));
    CHECK((ndijkstra(graph, 0, [](nidx_t) { return 1LL; }, 999LL) == vector<long long>{0, 1, 2, 999}));
}
