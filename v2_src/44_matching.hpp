struct nbipartite_matching {
    int size = 0;
    nvector<int> left, right;
};

template <ngraph_like G> nbipartite_matching nhopcroft_karp(const G& graph, int right_vertices) {
    int left_vertices = ni::ngraph_vertices(graph);
    npre(right_vertices >= 0);
    nvector<nvector<int>> adjacency(left_vertices);
    for (int left = 0; left < left_vertices; ++left) {
        decltype(auto) edges = graph.neighbors(left);
        nfor(edge, edges) {
            int right = nedge_to(edge);
            npre(0 <= right && right < right_vertices);
            adjacency[left].push(right);
        }
    }

    nbipartite_matching result{0, nvector<int>(left_vertices, npos), nvector<int>(right_vertices, npos)};
    nvector<int> distance(left_vertices), next(left_vertices);
    for (;;) {
        for (int left = 0; left < left_vertices; ++left)
            distance[left] = npos;
        ndeque<int> queue;
        for (int left = 0; left < left_vertices; ++left)
            if (result.left[left] == npos) {
                distance[left] = 0;
                queue.pushr(left);
            }
        long long shortest = numeric_limits<long long>::max();
        while (!queue.empty()) {
            int left = queue.popl();
            if (distance[left] + 1 > shortest)
                continue;
            for (int i = 0; i < adjacency[left].len(); ++i) {
                int right = adjacency[left][i], next_left = result.right[right];
                if (next_left == npos)
                    nchmin(shortest, distance[left] + 1);
                else if (distance[next_left] == npos && distance[left] + 1 < shortest) {
                    distance[next_left] = distance[left] + 1;
                    queue.pushr(next_left);
                }
            }
        }
        if (shortest == numeric_limits<long long>::max())
            break;
        for (int left = 0; left < left_vertices; ++left)
            next[left] = 0;
        int augmented = 0;
        for (int root = 0; root < left_vertices; ++root) {
            if (result.left[root] != npos || distance[root] != 0)
                continue;
            nvector<int> stack{root}, via_right{npos};
            bool found = false;
            while (!stack.empty() && !found) {
                int left = stack.back();
                while (next[left] < adjacency[left].len()) {
                    int right = adjacency[left][next[left]++];
                    int next_left = result.right[right];
                    if (next_left == npos) {
                        if (distance[left] + 1 != shortest)
                            continue;
                        result.left[left] = right;
                        result.right[right] = left;
                        for (int depth = stack.len() - 1; depth > 0; --depth) {
                            int parent = stack[depth - 1], parent_right = via_right[depth];
                            result.left[parent] = parent_right;
                            result.right[parent_right] = parent;
                        }
                        found = true;
                        ++augmented;
                        break;
                    }
                    if (distance[next_left] == distance[left] + 1) {
                        stack.push(next_left);
                        via_right.push(right);
                        break;
                    }
                }
                if (!found && !stack.empty() && next[stack.back()] == adjacency[stack.back()].len()) {
                    distance[stack.back()] = npos;
                    stack.pop();
                    via_right.pop();
                }
            }
        }
        if (!augmented)
            break;
        result.size += augmented;
    }
    return result;
}
