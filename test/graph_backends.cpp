#include "common.hpp"

template <ngraph_like G> int check_shortest(const G& graph, const vector<vector<int>>& reference) {
    for (int source = 0; source < int(reference.size()); ++source) {
        auto distance = ndijkstra<int>(graph, source, 1'000'000);
        for (int vertex = 0; vertex < int(reference.size()); ++vertex)
            ntest(distance[vertex] == reference[source][vertex]);
    }
    return 0;
}

int main() {
    static_assert(ngraph_like<ngraph_forward<int>>);
    static_assert(ngraph_like<ngraph_csr<int>>);
    static_assert(same_as<ngraph<int>, ngraph_forward<int>>);

    ngraph<int> graph(4, 8);
    int edge01 = graph.add(0, 1, 2);
    int edge12 = graph.add(1, 2, 3);
    graph.add(0, 3, 9);
    ntest(graph.len() == 4 && graph.edges() == 3 && graph.degree(0) == 2);
    ntest(graph.find(0, 1) == edge01 && graph.find(2, 0) == npos);
    ntest(graph.find(2, 0, 77) == 77 && graph.has(1, 2) && !graph.has(2, 1));
    ntest(*graph.weight(edge12) == 3 && graph.weight(99) == nullptr && graph.weight(99, 8) == 8);
    ntest(graph.set(edge12, 4) && !graph.set(99, 1));

    int vertex_sum = 0, arc_count = 0;
    nfor(vertex, graph.vertices())
        vertex_sum += vertex;
    nfor(edge, graph.arcs()) {
        ntest(0 <= edge.from && edge.from < graph.len());
        ntest(0 <= edge.to && edge.to < graph.len());
        edge.w += 1;
        ++arc_count;
    }
    ntest(vertex_sum == 6 && arc_count == graph.edges());
    ntest(*graph.weight(edge01) == 3 && *graph.weight(edge12) == 5);
    ntest(ndijkstra(graph, 0)[2] == 8);

    ngraph_csr csr(graph);
    static_assert(same_as<decltype(csr), ngraph_csr<int>>);
    ntest(csr.len() == graph.len() && csr.edges() == graph.edges());
    ntest(csr.degree(0) == 2 && csr.has(0, 1) && ndijkstra(csr, 0)[2] == 8);
    int csr01 = csr.find(0, 1);
    ntest(csr.set(csr01, 4) && *csr.weight(csr01) == 4);
    auto reversed = csr.reverse();
    ntest(reversed.has(1, 0) && reversed.has(2, 1));
    ntest(nbfs(reversed, 2)[0] != npos);

    mt19937 random(0x4001U);
    for (int repeat = 0; repeat < 180; ++repeat) {
        int vertices = 2 + int(random() % 18);
        ngraph_forward<int> forward(vertices);
        vector<vector<int>> distance(size_t(vertices), vector<int>(size_t(vertices), 1'000'000));
        for (int vertex = 0; vertex < vertices; ++vertex)
            distance[vertex][vertex] = 0;
        for (int attempt = 0; attempt < vertices * vertices / 3; ++attempt) {
            int from = int(random() % vertices), to = int(random() % vertices);
            int weight = int(random() % 21);
            forward.add(from, to, weight);
            distance[from][to] = min(distance[from][to], weight);
        }
        for (int middle = 0; middle < vertices; ++middle)
            for (int from = 0; from < vertices; ++from)
                for (int to = 0; to < vertices; ++to)
                    distance[from][to] = min(distance[from][to],
                                             distance[from][middle] + distance[middle][to]);
        ngraph_csr<int> static_graph(forward);
        ntest(check_shortest(forward, distance) == 0);
        ntest(check_shortest(static_graph, distance) == 0);
    }

    nflow<long long> limited(6);
    limited.add(0, 1, 16);
    limited.add(0, 2, 13);
    limited.add(1, 2, 10);
    limited.add(2, 1, 4);
    limited.add(1, 3, 12);
    limited.add(3, 2, 9);
    limited.add(2, 4, 14);
    limited.add(4, 3, 7);
    limited.add(3, 5, 20);
    limited.add(4, 5, 4);
    ntest(limited(0, 5, 10) == 10);
    ntest(limited.flow(0, 5) == 13);
    auto cut = limited.cut(0);
    ntest(cut[0] && !cut[5]);
    limited.reset();
    ntest(limited(0, 5) == 23);

    for (int repeat = 0; repeat < 250; ++repeat) {
        int vertices = 2 + int(random() % 9);
        int source = 0, sink = vertices - 1;
        nflow_dinic<long long> dinic(vertices);
        nmaxflow<long long> push_relabel(vertices);
        for (int attempt = 0; attempt < vertices * vertices; ++attempt) {
            int from = int(random() % vertices), to = int(random() % vertices);
            if (from == to)
                continue;
            long long capacity = random() % 20;
            dinic.add(from, to, capacity);
            push_relabel.add(from, to, capacity);
        }
        long long expected = push_relabel.flow(source, sink);
        ntest(dinic.flow(source, sink) == expected);
        dinic.reset();
        ntest(dinic.flow(source, sink) == expected);
    }
}
