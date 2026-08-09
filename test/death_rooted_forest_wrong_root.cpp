#include "common.hpp"

int main() {
    ngraph_list<int> graph(3);
    graph.add(1, 0);
    graph.add(1, 2);
    nrooted_forest<int> invalid(graph, 0);
    return invalid.len();
}
