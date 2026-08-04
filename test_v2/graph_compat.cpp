#include "common.hpp"

int main() {
    nvector<nvector<narc<int>>> adjacency(5);
    auto add2 = [&](int a, int b, int weight) {
        adjacency[a].push(narc<int>{b, weight});
        adjacency[b].push(narc<int>{a, weight});
    };
    add2(0, 1, 4);
    add2(0, 2, 2);
    add2(1, 2, 1);
    add2(1, 3, 5);
    add2(2, 3, 8);
    add2(3, 4, 2);

    auto graph = ngraph_view(5, [&](int vertex) {
        return nview(adjacency[vertex].len(),
                     [&, vertex](int index) -> const narc<int>& {
                         return adjacency[vertex][index];
                     });
    });
    ntest(ncollect(nvertices(graph)) == nvector<int>({0, 1, 2, 3, 4}));
    auto all_arcs = narcs(graph);
    int count = 0, weight_sum = 0;
    nfor(edge, all_arcs) {
        ntest(0 <= edge.from && edge.from < 5 && 0 <= edge.to && edge.to < 5);
        weight_sum += edge.w;
        ++count;
    }
    ntest(count == 12 && weight_sum == 44);

    auto bfs = nbfs_path(graph, 0);
    ntest(bfs.reach(4));
    auto bfs_path = bfs.path(4);
    ntest(!bfs_path.empty() && bfs_path[0] == 0 && bfs_path.back() == 4);
    ntest(bfs.dist(4, -1) == 3 && bfs.dist(99, -1) == -1);

    auto shortest = ndijkstra_path(graph, 0, 1'000'000LL);
    ntest(shortest[4] == 10);
    ntest(shortest.path(4) == nvector<int>({0, 2, 1, 3, 4}));

    auto mst = nkruskal(graph);
    ntest(mst.connected() && mst.components == 1 && mst.weight == 10);
    ntest(mst.edges.len() == 4 && mst.edge.len() == 4);

    ngraph_list<int> disconnected(4);
    disconnected.add2(0, 1, 3);
    disconnected.add2(2, 3, 7);
    auto forest = nkruskal(disconnected);
    ntest(!forest.connected() && forest.components == 2 && forest.weight == 10);
}
