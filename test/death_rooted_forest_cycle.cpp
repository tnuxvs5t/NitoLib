#include "common.hpp"

int main() {
    ngraph_list<int> graph(3);
    graph.add2(0, 1);
    graph.add2(1, 2);
    graph.add2(2, 0);
    nrooted_forest<int> invalid(graph);
    return invalid.len();
}
