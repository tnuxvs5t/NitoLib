#include "common.hpp"

int main() {
    auto graph = ngraph_view(1, [](int) { return nvector<unsigned long long>{1ULL << 32}; });
    return nbfs(graph, 0)[0];
}
