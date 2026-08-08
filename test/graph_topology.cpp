#include "common.hpp"

static vector<int> adjacency_ids(const ngraph_topology<int, int>& graph, int vertex) {
    vector<int> result;
    nfor(edge, graph.neighbors(vertex))
        result.push_back(edge.id);
    return result;
}

static vector<int> arc_ids(const ngraph_topology<int, int>& graph) {
    vector<int> result;
    nfor(edge, graph.arcs())
        result.push_back(edge.id);
    return result;
}

int main() {
    static_assert(ngraph_like<ngraph_topology<int, int>>);
    static_assert(same_as<ngraph_topology<int>::weight_type, int>);

    ngraph_topology<int, int> graph(3, 8);
    ntest(graph.len() == 3 && graph.vertices().len() == 3 && !graph.empty());
    ntest(graph.vertex_value(0) == 0 && graph.vertex_node(0).val().kind == ngraph_node_kind::vertex);

    int loop = graph.add(0, 0, 5);
    int parallel = graph.add(0, 1, 7);
    int outgoing = graph.add(0, 2, 9);
    ntest(loop == 0 && parallel == 1 && outgoing == 2);
    ntest(graph.edges() == 3 && graph.degree(0) == 3);
    ntest(adjacency_ids(graph, 0) == vector<int>({loop, parallel, outgoing}));
    ntest(arc_ids(graph) == vector<int>({loop, parallel, outgoing}));
    ntest(graph.find(0, 1) == parallel && graph.has(0, 2) && !graph.has(1, 0));

    auto vertex_view = graph.vertex_node(0);
    auto arc_view = graph.arc_node(parallel);
    nnode_identity vertex_identity = vertex_view.identity();
    nnode_identity arc_identity = arc_view.identity();
    ntest(vertex_view.current() && arc_view.current());
    ntest(vertex_view.count() == 1 && vertex_view.len() == 1 && vertex_view.leaf());
    ntest(vertex_identity != arc_identity && vertex_view.same_domain(arc_view));

    ntest(graph.set(parallel, 11));
    ntest(arc_view.current() && arc_view.val().weight == 11);
    nfor(edge, graph.arcs())
        if (edge.id == outgoing)
            edge.w += 3;
    ntest(graph.weight(outgoing, -1) == 12);
    graph.set_vertex(0, 42);
    ntest(vertex_view.current() && vertex_view.val().payload && *vertex_view.val().payload == 42);

    ntest(graph.erase(loop));
    ntest(!vertex_view.current() && !arc_view.current());
    ntest(graph.edges() == 2 && graph.degree(0) == 2);
    ntest(adjacency_ids(graph, 0) == vector<int>({parallel, outgoing}));
    ntest(arc_ids(graph) == vector<int>({parallel, outgoing}));
    ntest(!graph.erase(loop) && graph.weight(loop) == nullptr);

    int appended = graph.add(1, 2, 13);
    ntest(appended == 3 && graph.edges() == 3);
    auto moved_arc = graph.arc_node(appended);
    ntest(graph.rewire(appended, 2, 1));
    ntest(!moved_arc.current() && graph.find(2, 1) == appended);
    ntest(graph.degree(1) == 0 && graph.degree(2) == 1);

    graph.clear_edges();
    ntest(graph.edges() == 0 && graph.arcs().empty());
    int after_edge_clear = graph.add(0, 1, 17);
    ntest(after_edge_clear == 4);
    graph.clear();
    ntest(graph.empty() && graph.edges() == 0);
    int new_vertex = graph.add_vertex(99);
    ntest(new_vertex == 0 && graph.vertex_value(0) == 99);
    ntest(graph.add(0, 0) == 0);

    ngraph_topology<int> first(2);
    auto shared_domain = first.domain();
    auto first_view = first.vertex_node(0);
    ngraph_topology<int> second(shared_domain, 2);
    ntest(first.same_domain(second));
    ntest(!first_view.current());
    int shared_arc = second.add(0, 1, 3);
    auto second_arc = second.arc_node(shared_arc);
    first.clear();
    ntest(second.edges() == 1 && !second_arc.current());
    auto current_second_arc = second.arc_node(shared_arc);
    ntest(current_second_arc.current() && current_second_arc.val().from == 0);
    second.clear();
    ntest(second.empty() && first.empty());

    ngraph_topology<int> original(1);
    int original_arc = original.add(0, 0, 8);
    auto original_view = original.arc_node(original_arc);
    ngraph_topology<int> copy = original;
    ntest(!original.same_domain(copy) && original_view.current());
    ntest(copy.arc_node(original_arc).current());
    ngraph_topology<int> moved = move(original);
    ntest(!original_view.current() && moved.arc_node(original_arc).current());
}
