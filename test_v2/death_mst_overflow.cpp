#include "common.hpp"

int main() {
    ngraph_list<int> graph(3);
    graph.add2(0, 1, INT_MAX);
    graph.add2(1, 2, INT_MAX);
    return nprim<int>(graph)->weight;
}
