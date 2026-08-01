template <ngraph_like G, class T, class Merge, class Vertex, class Lift>
    requires copyable<T>
nvector<T> nreroot(const G& graph, T identity, Merge merge, Vertex vertex, Lift lift, int root = 0) {
    int vertices = ni::ngraph_vertices(graph);
    if (!vertices)
        return {};
    auto layout = ni::nbuild_tree_layout(graph, root, true);
    auto& adjacency = layout.adjacency;
    auto& parent = layout.parent;
    auto& order = layout.order;

    nvector<T> down(vertices, identity), upward(vertices, identity), answer(vertices, identity);
    for (int position = vertices; position-- > 0;) {
        int from = order[position];
        T aggregate = identity;
        for (int to : adjacency[from])
            if (parent[to] == from)
                aggregate = invoke(merge, move(aggregate), invoke(lift, down[to], to, from));
        down[from] = invoke(vertex, move(aggregate), from);
    }

    for (int position = 0; position < vertices; ++position) {
        int from = order[position];
        int degree = int(adjacency[from].size());
        nvector<T> contribution;
        contribution.reserve(degree);
        for (int to : adjacency[from])
            contribution.push(to == parent[from] ? upward[from] : invoke(lift, down[to], to, from));

        nvector<T> prefix(degree + 1, identity), suffix(degree + 1, identity);
        for (int i = 0; i < degree; ++i)
            prefix[i + 1] = invoke(merge, prefix[i], contribution[i]);
        for (int i = degree; i-- > 0;)
            suffix[i] = invoke(merge, contribution[i], suffix[i + 1]);
        answer[from] = invoke(vertex, prefix[degree], from);

        for (int i = 0; i < degree; ++i) {
            int to = adjacency[from][i];
            if (parent[to] != from)
                continue;
            T without_child = invoke(merge, prefix[i], suffix[i + 1]);
            upward[to] = invoke(lift, invoke(vertex, move(without_child), from), from, to);
        }
    }
    return answer;
}
