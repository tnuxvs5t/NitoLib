#include "common.hpp"

int main() {
    ngraph_list<double> graph(1);
    return ndijkstra<double>(graph, 0, numeric_limits<double>::quiet_NaN())[0] != 0;
}
