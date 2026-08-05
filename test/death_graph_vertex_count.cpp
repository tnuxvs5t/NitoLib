#include "common.hpp"

struct nhuge_graph {
    unsigned long long vertices() const { return 1ULL << 32; }
    nvector<int> neighbors(int) const { return {}; }
};

int main() { return nbfs(nhuge_graph{}, 0).len(); }
