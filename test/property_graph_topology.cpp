#include "common.hpp"

struct nreference_topology_arc {
    int from = 0, to = 0, weight = 0;
    bool live = false;
};

static void erase_id(vector<int>& ids, int id) {
    auto it = find(ids.begin(), ids.end(), id);
    if (it != ids.end())
        ids.erase(it);
}

static int reference_find(const vector<vector<int>>& adjacency,
                          const vector<nreference_topology_arc>& arcs, int from, int to) {
    for (int id : adjacency[size_t(from)])
        if (arcs[size_t(id)].live && arcs[size_t(id)].to == to)
            return id;
    return npos;
}

static int check_topology(const ngraph_topology<int, int>& graph,
                          const vector<vector<int>>& adjacency,
                          const vector<nreference_topology_arc>& arcs) {
    int vertices = int(adjacency.size());
    ntest(graph.len() == vertices && graph.vertices().len() == vertices);

    int live_arcs = 0;
    vector<int> expected_arcs;
    for (int id = 0; id < int(arcs.size()); ++id) {
        const auto& arc = arcs[size_t(id)];
        if (!arc.live)
            continue;
        ++live_arcs;
        expected_arcs.push_back(id);
        ntest(graph.weight(id, -1) == arc.weight);
        auto view = graph.arc_node(id);
        ntest(view.current() && view.val().kind == ngraph_node_kind::arc);
        ntest(view.val().id == id && view.val().from == arc.from && view.val().to == arc.to);
    }
    ntest(graph.edges() == live_arcs && graph.arcs().len() == live_arcs);

    vector<int> actual_arcs;
    nfor(edge, graph.arcs()) {
        actual_arcs.push_back(edge.id);
        ntest(arcs[size_t(edge.id)].live);
        ntest(edge.from == arcs[size_t(edge.id)].from && edge.to == arcs[size_t(edge.id)].to);
        ntest(edge.w == arcs[size_t(edge.id)].weight);
    }
    ntest(actual_arcs == expected_arcs);

    for (int vertex = 0; vertex < vertices; ++vertex) {
        const auto& expected = adjacency[size_t(vertex)];
        ntest(graph.degree(vertex) == int(expected.size()));
        vector<int> actual;
        nfor(edge, graph.neighbors(vertex)) {
            actual.push_back(edge.id);
            const auto& reference = arcs[size_t(edge.id)];
            ntest(reference.live && edge.from == vertex);
            ntest(edge.to == reference.to && edge.w == reference.weight);
        }
        ntest(actual == expected);
        for (int to = 0; to < vertices; ++to) {
            int expected_id = reference_find(adjacency, arcs, vertex, to);
            ntest(graph.find(vertex, to, -77) == (expected_id == npos ? -77 : expected_id));
            ntest(graph.has(vertex, to) == (expected_id != npos));
        }
    }
    return 0;
}

static int check_algorithms(const ngraph_topology<int, int>& graph) {
    int vertices = graph.len();
    if (!vertices)
        return 0;

    ngraph_list<int> list(vertices);
    ngraph_forward<int> forward(vertices);
    nfor(edge, graph.arcs()) {
        list.add(edge.from, edge.to, edge.w);
        forward.add(edge.from, edge.to, edge.w);
    }
    ngraph_csr<int> csr(graph);
    for (int source = 0; source < vertices; ++source) {
        ntest(nbfs(graph, source) == nbfs(list, source));
        ntest(nbfs(graph, source) == nbfs(forward, source));
        ntest(nbfs(graph, source) == nbfs(csr, source));
        ntest(ndijkstra<long long>(graph, source) == ndijkstra<long long>(list, source));
        ntest(ndijkstra<long long>(graph, source) == ndijkstra<long long>(forward, source));
        ntest(ndijkstra<long long>(graph, source) == ndijkstra<long long>(csr, source));
    }
    return 0;
}

int main() {
    mt19937 random(0x9e3779b9U);
    for (int trial = 0; trial < 180; ++trial) {
        int initial_vertices = int(random() % 8);
        ngraph_topology<int, int> graph(initial_vertices, 48);
        vector<vector<int>> adjacency(static_cast<size_t>(initial_vertices));
        vector<nreference_topology_arc> arcs;

        for (int operation = 0; operation < 260; ++operation) {
            int vertices = int(adjacency.size());
            int kind = int(random() % 9);
            if (kind == 0 || (kind == 1 && !vertices)) {
                if (!vertices) {
                    ntest(graph.add_vertex() == 0);
                    adjacency.emplace_back();
                } else {
                    int from = int(random() % vertices), to = int(random() % vertices);
                    int weight = int(random() % 41);
                    int id = graph.add(from, to, weight);
                    ntest(id == int(arcs.size()));
                    arcs.push_back({from, to, weight, true});
                    adjacency[size_t(from)].push_back(id);
                }
            } else if (kind == 1) {
                if (!vertices)
                    continue;
                int from = int(random() % vertices), to = int(random() % vertices);
                int weight = int(random() % 41);
                auto ids = graph.add2(from, to, weight);
                ntest(ids.first == int(arcs.size()));
                arcs.push_back({from, to, weight, true});
                adjacency[size_t(from)].push_back(ids.first);
                ntest(ids.second == int(arcs.size()));
                arcs.push_back({to, from, weight, true});
                adjacency[size_t(to)].push_back(ids.second);
            } else if (kind == 2) {
                int id = arcs.empty() ? 0 : int(random() % (arcs.size() + 3));
                bool expected = id >= 0 && id < int(arcs.size()) && arcs[size_t(id)].live;
                ntest(graph.erase(id) == expected);
                if (expected) {
                    erase_id(adjacency[size_t(arcs[size_t(id)].from)], id);
                    arcs[size_t(id)].live = false;
                }
            } else if (kind == 3) {
                if (!vertices)
                    continue;
                int id = arcs.empty() ? 0 : int(random() % (arcs.size() + 3));
                bool expected = id >= 0 && id < int(arcs.size()) && arcs[size_t(id)].live;
                int from = int(random() % vertices), to = int(random() % vertices);
                ntest(graph.rewire(id, from, to) == expected);
                if (expected) {
                    auto& arc = arcs[size_t(id)];
                    if (arc.from != from || arc.to != to) {
                        erase_id(adjacency[size_t(arc.from)], id);
                        adjacency[size_t(from)].push_back(id);
                    }
                    arc.from = from;
                    arc.to = to;
                }
            } else if (kind == 4) {
                int id = arcs.empty() ? 0 : int(random() % (arcs.size() + 3));
                bool expected = id >= 0 && id < int(arcs.size()) && arcs[size_t(id)].live;
                int weight = int(random() % 41);
                ntest(graph.set(id, weight) == expected);
                if (expected)
                    arcs[size_t(id)].weight = weight;
            } else if (kind == 5) {
                int id = graph.add_vertex(int(random() % 101));
                ntest(id == vertices);
                adjacency.emplace_back();
            } else if (kind == 6) {
                graph.clear_edges();
                for (auto& ids : adjacency)
                    ids.clear();
                for (auto& arc : arcs)
                    arc.live = false;
            } else if (kind == 7) {
                graph.clear();
                adjacency.clear();
                arcs.clear();
            } else {
                if (!vertices)
                    continue;
                int vertex = int(random() % vertices);
                graph.set_vertex(vertex, int(random() % 101));
            }

            ntest(check_topology(graph, adjacency, arcs) == 0);
        }

        if (adjacency.empty()) {
            ntest(graph.add_vertex() == 0);
            adjacency.emplace_back();
            ntest(check_topology(graph, adjacency, arcs) == 0);
        }
        ntest(check_algorithms(graph) == 0);
    }
}
