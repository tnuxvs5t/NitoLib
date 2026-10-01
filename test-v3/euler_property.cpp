#include "../src-v3/graph_algo.hpp"
#include "../src-v3/graph_store.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct arc { nidx_t to, id; };
struct edge { nidx_t from, to; };

void verify(const vector<edge>& input, const neuler_result& trail,
            optional<nidx_t> start, bool directed) {
    nidx_t m = nidx_t(input.size());
    CHECK(trail.complete);
    CHECK(nidx_t(trail.edges.size()) == m);
    CHECK(nidx_t(trail.vertices.size()) == m + 1);
    if (start) CHECK(trail.vertices[0] == *start);
    vector<unsigned char> seen(m);
    for (nidx_t i = 0; i < m; ++i) {
        nidx_t id = trail.edges[i], from = trail.vertices[i], to = trail.vertices[i + 1];
        CHECK(0 <= id && id < m && !seen[id]);
        seen[id] = true;
        CHECK((input[id].from == from && input[id].to == to) ||
              (!directed && input[id].from == to && input[id].to == from));
    }
}

void verify_part(const vector<edge>& input, const neuler_result& trail,
                 nidx_t start, const vector<nidx_t>& wanted, bool directed) {
    CHECK(nidx_t(trail.edges.size()) == nidx_t(wanted.size()));
    CHECK(trail.vertices.size() == trail.edges.size() + 1);
    CHECK(trail.vertices.front() == start);
    CHECK(trail.complete == (wanted.size() == input.size()));
    vector<unsigned char> expected(input.size()), seen(input.size());
    for (nidx_t id : wanted) expected[id] = true;
    for (nidx_t i = 0; i < nidx_t(trail.edges.size()); ++i) {
        nidx_t id = trail.edges[i], from = trail.vertices[i], to = trail.vertices[i + 1];
        CHECK(0 <= id && id < nidx_t(input.size()) && expected[id] && !seen[id]);
        seen[id] = true;
        CHECK((input[id].from == from && input[id].to == to) ||
              (!directed && input[id].from == to && input[id].to == from));
    }
}

int main() {
    auto empty = ngraph{0, [](nidx_t) { return vector<arc>{}; },
                        [](const arc& e) { return e.to; }, [](const arc& e) { return e.id; }};
    auto empty_result = neuler(empty, 0);
    CHECK(empty_result.complete && empty_result.vertices.empty() &&
          empty_result.edges.empty());
    vector<vector<arc>> vacant(3);
    auto no_edges = ngraph{3, [&](nidx_t v) -> auto& { return vacant[v]; },
                           [](const arc& e) { return e.to; }, [](const arc& e) { return e.id; }};
    CHECK((neuler(no_edges, 0).vertices == vector<nidx_t>{0}));
    CHECK((neuler(no_edges, 2, 0).vertices == vector<nidx_t>{2}));

    vector<edge> fixed{{0, 1}, {0, 1}, {1, 1}};
    vector<vector<arc>> adjacency(3);
    for (nidx_t i = 0; i < nidx_t(fixed.size()); ++i) {
        auto [from, to] = fixed[i];
        adjacency[from].push_back({to, i});
        adjacency[to].push_back({from, i});
    }
    auto undirected = ngraph{3, [&](nidx_t v) -> auto& { return adjacency[v]; },
                             [](const arc& e) { return e.to; }, [](const arc& e) { return e.id; }};
    verify(fixed, neuler(undirected, 3), nullopt, false);
    verify(fixed, neuler(undirected, 0, 3), 0, false);
    auto wrong_count = neuler(undirected, 2);
    CHECK(!wrong_count.complete && wrong_count.edges.size() == 3);

    auto csr = nmake_undirected_csr(3, fixed,
        [](const edge& e) { return e.from; }, [](const edge& e) { return e.to; });
    verify(fixed, neuler(csr, 3), nullopt, false);

    vector<vector<arc>> keyed_adjacency{{}, {{0, 1}}, {{1, 0}}};
    auto keyed = ngraph{nreverse(nrange(3)),
        [&](nidx_t key) { return keyed_adjacency[key]; },
        [](const arc& e) { return e.to; }, [](const arc& e) { return e.id; }};
    auto keyed_trail = neuler(keyed, 2, 2, neuler_kind::directed);
    CHECK((keyed_trail.vertices == vector<nidx_t>{0, 1, 2}));
    CHECK((keyed_trail.edges == vector<nidx_t>{0, 1}));
    CHECK(keyed_trail.complete);

    vector<vector<arc>> sparse_adjacency(2);
    sparse_adjacency[0] = {{1, 1000000000}};
    sparse_adjacency[1] = {{0, 1000000000}};
    auto sparse = ngraph{2, [&](nidx_t v) -> auto& { return sparse_adjacency[v]; },
                         [](const arc& e) { return e.to; }, [](const arc& e) { return e.id; }};
    auto sparse_trail = neuler(sparse, 1);
    CHECK(sparse_trail.complete && sparse_trail.edges == vector<nidx_t>{1000000000});

    vector<vector<arc>> disconnected(5);
    disconnected[0] = {{1, 10}};
    disconnected[1] = {{0, 10}};
    disconnected[2] = {{3, 20}};
    disconnected[3] = {{2, 20}};
    auto disconnected_graph = ngraph{
        5, [&](nidx_t v) -> auto& { return disconnected[v]; },
        [](const arc& e) { return e.to; }, [](const arc& e) { return e.id; }};
    auto first_part = neuler(disconnected_graph, 2);
    CHECK(!first_part.complete && first_part.vertices == vector<nidx_t>({0, 1}) &&
          first_part.edges == vector<nidx_t>({10}));
    auto second_part = neuler(disconnected_graph, 2, 2);
    CHECK(!second_part.complete && second_part.vertices == vector<nidx_t>({2, 3}) &&
          second_part.edges == vector<nidx_t>({20}));
    auto isolated = neuler(disconnected_graph, 4, 2);
    CHECK(!isolated.complete && isolated.vertices == vector<nidx_t>({4}) &&
          isolated.edges.empty());

    vector<vector<arc>> disjoint_loops{{{0, 5}, {0, 5}}, {{1, 9}, {1, 9}}};
    auto loop_graph = ngraph{2, [&](nidx_t v) -> auto& { return disjoint_loops[v]; },
                             [](const arc& e) { return e.to; },
                             [](const arc& e) { return e.id; }};
    auto loop_part = neuler(loop_graph, 2);
    CHECK(!loop_part.complete && loop_part.vertices == vector<nidx_t>({0, 0}) &&
          loop_part.edges == vector<nidx_t>({5}));
    auto other_loop = neuler(loop_graph, 1, 2);
    CHECK(!other_loop.complete && other_loop.vertices == vector<nidx_t>({1, 1}) &&
          other_loop.edges == vector<nidx_t>({9}));

    vector<vector<arc>> directed_parts(4);
    directed_parts[0] = {{1, 4}};
    directed_parts[2] = {{3, 8}};
    auto directed_graph = ngraph{
        4, [&](nidx_t v) -> auto& { return directed_parts[v]; },
        [](const arc& e) { return e.to; }, [](const arc& e) { return e.id; }};
    auto directed_part = neuler(directed_graph, 2, neuler_kind::directed);
    CHECK(!directed_part.complete && directed_part.vertices == vector<nidx_t>({0, 1}) &&
          directed_part.edges == vector<nidx_t>({4}));

    vector<vector<arc>> star(4);
    for (nidx_t to = 1; to < 4; ++to) {
        star[0].push_back({to, to - 1});
        star[to].push_back({0, to - 1});
    }
    auto star_graph = ngraph{4, [&](nidx_t v) -> auto& { return star[v]; },
                             [](const arc& e) { return e.to; },
                             [](const arc& e) { return e.id; }};
    auto unchecked = neuler(star_graph, 3);
    CHECK(unchecked.complete && unchecked.edges.size() == 3); // Not a trail certificate.

    mt19937 rng(0xE001E7);
    for (nidx_t trial = 0; trial < 5000; ++trial) {
        nidx_t n = 1 + nidx_t(rng() % 7), m = nidx_t(rng() % 12);
        bool directed = bool(rng() & 1);
        vector<nidx_t> walk(m + 1);
        for (nidx_t& v : walk) v = nidx_t(rng() % n);
        vector<edge> input(m);
        vector<vector<arc>> buckets(n);
        for (nidx_t i = 0; i < m; ++i) {
            input[i] = {walk[i], walk[i + 1]};
            buckets[walk[i]].push_back({walk[i + 1], i});
            if (!directed) buckets[walk[i + 1]].push_back({walk[i], i});
        }
        for (auto& bucket : buckets) shuffle(bucket.begin(), bucket.end(), rng);
        auto graph = ngraph{n, [&](nidx_t v) -> auto& { return buckets[v]; },
                            [](const arc& e) { return e.to; }, [](const arc& e) { return e.id; }};
        auto kind = directed ? neuler_kind::directed : neuler_kind::undirected;
        verify(input, neuler(graph, m, kind), nullopt, directed);
        verify(input, neuler(graph, walk[0], m, kind), walk[0], directed);
    }

    for (nidx_t trial = 0; trial < 2000; ++trial) {
        bool directed = bool(rng() & 1);
        nidx_t components = 2 + nidx_t(rng() % 3), n = 0;
        vector<edge> input;
        vector<vector<arc>> buckets;
        vector<vector<nidx_t>> wanted(components);
        vector<nidx_t> starts(components), owner;
        for (nidx_t component = 0; component < components; ++component) {
            nidx_t size = 1 + nidx_t(rng() % 4), base = n;
            n += size;
            buckets.resize(n);
            owner.resize(n, component);
            nidx_t count = 1 + nidx_t(rng() % 6);
            vector<nidx_t> walk(count + 1);
            for (nidx_t& vertex : walk) vertex = base + nidx_t(rng() % size);
            starts[component] = walk[0];
            for (nidx_t i = 0; i < count; ++i) {
                nidx_t id = nidx_t(input.size());
                input.push_back({walk[i], walk[i + 1]});
                wanted[component].push_back(id);
                buckets[walk[i]].push_back({walk[i + 1], id});
                if (!directed) buckets[walk[i + 1]].push_back({walk[i], id});
            }
        }
        for (auto& bucket : buckets) shuffle(bucket.begin(), bucket.end(), rng);
        auto graph = ngraph{n, [&](nidx_t vertex) -> auto& { return buckets[vertex]; },
                            [](const arc& e) { return e.to; },
                            [](const arc& e) { return e.id; }};
        auto kind = directed ? neuler_kind::directed : neuler_kind::undirected;
        nidx_t m = nidx_t(input.size());
        for (nidx_t component = 0; component < components; ++component)
            verify_part(input, neuler(graph, starts[component], m, kind),
                        starts[component], wanted[component], directed);
        auto automatic = neuler(graph, m, kind);
        verify_part(input, automatic, automatic.vertices.front(),
                    wanted[owner[automatic.vertices.front()]], directed);
    }
}
