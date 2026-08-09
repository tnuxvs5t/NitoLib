#include "common.hpp"

int main() {
    ngraph_list<int> graph(3);
    graph.add(0, 1);  // missing reverse in this component
    graph.add2(1, 2); // symmetric representation in the same component
    nrooted_forest<int> invalid(graph, 0, false);
    return invalid.len();
}
