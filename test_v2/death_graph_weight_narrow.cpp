#include "common.hpp"

int main() {
    ngraph_list<unsigned long long> graph(2);
    graph.add(0, 1, ULLONG_MAX);
    return ndijkstra<long long>(graph, 0)[1];
}
