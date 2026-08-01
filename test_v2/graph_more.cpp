#include "common.hpp"

int main() {
    ngraph_list<int> binary(6);
    binary.add(0, 1, 0);
    binary.add(0, 2, 1);
    binary.add(1, 2, 0);
    binary.add(1, 3, 1);
    binary.add(2, 4, 1);
    binary.add(3, 5, 1);
    binary.add(4, 5, 0);
    ntest((n01bfs(binary, 0) == nvector<int>{0, 0, 0, 1, 1, 1}));

    ngraph_list<int> weighted(5);
    weighted.add2(0, 1, 4);
    weighted.add2(0, 2, 2);
    weighted.add2(1, 2, 1);
    weighted.add2(1, 3, 5);
    weighted.add2(2, 3, 8);
    weighted.add2(2, 4, 10);
    weighted.add2(3, 4, 2);
    auto mst = nprim(weighted);
    ntest(mst && mst->weight == 10 && mst->edges.len() == 4);

    ngraph_list<int> disconnected(3);
    disconnected.add2(0, 1, 1);
    ntest(!nprim(disconnected));

    nmaxflow<long long> flow(6);
    flow.add(0, 1, 16);
    flow.add(0, 2, 13);
    flow.add(1, 2, 10);
    flow.add(2, 1, 4);
    flow.add(1, 3, 12);
    flow.add(3, 2, 9);
    flow.add(2, 4, 14);
    flow.add(4, 3, 7);
    flow.add(3, 5, 20);
    flow.add(4, 5, 4);
    ntest(flow.flow(0, 5) == 23);
    auto cut = flow.mincut(0);
    ntest(cut[0] && !cut[5]);
}
