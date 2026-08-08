#include "common.hpp"

int main() {
    ngraph_topology<int> graph(2);
    int id = graph.add(0, 1, 3);
    auto stale = graph.arc_node(id);
    graph.rewire(id, 1, 0);
    stale.val();
}
