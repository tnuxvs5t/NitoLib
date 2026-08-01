#include "common.hpp"

int main() {
    ngraph_list<int> dag(6);
    dag.add(0, 1);
    dag.add(0, 2);
    dag.add(1, 3);
    dag.add(2, 3);
    dag.add(3, 4);
    dag.add(2, 5);
    auto topological = ntoposort(dag);
    ntest(topological && topological->len() == 6);
    nvector<int> position(6);
    for (int i = 0; i < 6; ++i)
        position[(*topological)[i]] = i;
    for (int from = 0; from < 6; ++from) {
        auto adjacency = dag.neighbors(from);
        nfor(edge, adjacency) ntest(position[from] < position[edge.to]);
    }
    dag.add(4, 1);
    ntest(!ntoposort(dag));

    ngraph_list<int> directed(8);
    directed.add(0, 1);
    directed.add(1, 2);
    directed.add(2, 0);
    directed.add(2, 3);
    directed.add(3, 4);
    directed.add(4, 3);
    directed.add(4, 5);
    directed.add(5, 6);
    directed.add(6, 7);
    directed.add(7, 6);
    auto components = nscc(directed);
    ntest(components.classes() == 4);
    ntest(components.same(0, 2) && components.same(3, 4) && components.same(6, 7));
    ntest(!components.same(2, 3) && !components.same(4, 5));

    ngraph_list<int> tree(9);
    tree.add2(0, 1);
    tree.add2(0, 2);
    tree.add2(1, 3);
    tree.add2(1, 4);
    tree.add2(2, 5);
    tree.add2(5, 6);
    tree.add2(5, 7);
    tree.add2(7, 8);
    nlca lca(tree);
    ntest(lca(3, 4) == 1 && lca(3, 6) == 0 && lca(6, 8) == 5);
    ntest(lca.distance(3, 8) == 6);
    ntest(lca.jump(8, 2) == 5 && lca.jump(8, 5) == npos);
    ntest(lca.kth_on_path(3, 8, 0) == 3 && lca.kth_on_path(3, 8, 6) == 8);
}
