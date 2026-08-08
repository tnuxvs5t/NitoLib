#include "common.hpp"

struct nlegacy_forward_edge {
    int from = 0, to = 0, weight = 0;
};

struct nlegacy_forward_reference {
    int vertices;
    vector<vector<int>> adjacency;
    vector<nlegacy_forward_edge> edges;

    explicit nlegacy_forward_reference(int vertices)
        : vertices(vertices), adjacency(static_cast<size_t>(vertices)) {}

    int add(int from, int to, int weight) {
        int id = int(edges.size());
        edges.push_back({from, to, weight});
        adjacency[size_t(from)].insert(adjacency[size_t(from)].begin(), id);
        return id;
    }
    void clear_edges() {
        edges.clear();
        for (auto& ids : adjacency)
            ids.clear();
    }
    nlegacy_forward_reference reverse() const {
        nlegacy_forward_reference result(vertices);
        for (int vertex = 0; vertex < vertices; ++vertex)
            for (int id : adjacency[size_t(vertex)]) {
                const auto& edge = edges[size_t(id)];
                result.add(edge.to, edge.from, edge.weight);
            }
        return result;
    }
};

static int check_forward(const ngraph_forward<int>& graph,
                         const nlegacy_forward_reference& reference) {
    ntest(graph.len() == reference.vertices && graph.edges() == int(reference.edges.size()));
    vector<int> expected_arcs;
    for (int vertex = 0; vertex < reference.vertices; ++vertex) {
        const auto& expected = reference.adjacency[size_t(vertex)];
        ntest(graph.degree(vertex) == int(expected.size()));
        vector<int> actual;
        nfor(edge, graph.neighbors(vertex)) {
            actual.push_back(edge.id);
            const auto& reference_edge = reference.edges[size_t(edge.id)];
            ntest(edge.from == vertex && edge.to == reference_edge.to &&
                  edge.w == reference_edge.weight);
        }
        ntest(actual == expected);
        for (int to = 0; to < reference.vertices; ++to) {
            int expected_id = npos;
            for (int id : expected)
                if (reference.edges[size_t(id)].to == to) {
                    expected_id = id;
                    break;
                }
            ntest(graph.find(vertex, to, -77) == (expected_id == npos ? -77 : expected_id));
        }
        expected_arcs.insert(expected_arcs.end(), expected.begin(), expected.end());
    }

    vector<int> actual_arcs;
    nfor(edge, graph.arcs()) {
        actual_arcs.push_back(edge.id);
        ntest(reference.edges[size_t(edge.id)].from == edge.from);
    }
    ntest(actual_arcs == expected_arcs);

    vector<int> flattened;
    nfor(edge, narcs(graph))
        flattened.push_back(edge.id);
    ntest(flattened == expected_arcs);
    return 0;
}

int main() {
    mt19937 random(0x31415926U);
    for (int trial = 0; trial < 160; ++trial) {
        int vertices = int(random() % 9);
        ngraph_forward<int> graph(vertices, 64);
        nlegacy_forward_reference reference(vertices);
        for (int operation = 0; operation < 220; ++operation) {
            int kind = int(random() % 8);
            if (kind <= 3 && vertices) {
                int from = int(random() % vertices), to = int(random() % vertices);
                int weight = int(random() % 51);
                if (kind == 3) {
                    auto ids = graph.add2(from, to, weight);
                    ntest(ids.first == reference.add(from, to, weight));
                    ntest(ids.second == reference.add(to, from, weight));
                } else {
                    ntest(graph.add(from, to, weight) == reference.add(from, to, weight));
                }
            } else if (kind == 4) {
                graph.clear_edges();
                reference.clear_edges();
            } else if (kind == 5 && !reference.edges.empty()) {
                int id = int(random() % reference.edges.size());
                int weight = int(random() % 51);
                ntest(graph.set(id, weight));
                reference.edges[size_t(id)].weight = weight;
            } else if (kind == 6 && !reference.edges.empty()) {
                int id = int(random() % reference.edges.size());
                int weight = int(random() % 51);
                nfor(edge, graph.arcs())
                    if (edge.id == id)
                        edge.w = weight;
                reference.edges[size_t(id)].weight = weight;
                ntest(graph.weight(id, -1) == weight);
            }
            ntest(check_forward(graph, reference) == 0);

            if (operation % 17 == 0) {
                auto reversed = graph.reverse();
                auto expected = reference.reverse();
                ntest(check_forward(reversed, expected) == 0);
            }
        }
    }

    ngraph_forward<int> graph(3);
    ntest(graph.add(0, 1, 4) == 0);
    ntest(graph.add(0, 2, 5) == 1);
    graph.clear_edges();
    ntest(graph.add(0, 1, 6) == 0);
    ntest(graph.weight(1) == nullptr && graph.weight(0, -1) == 6);
}
