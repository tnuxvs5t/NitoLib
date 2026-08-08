// Topological sort requires a directed graph; a cycle yields an empty nmaybe rather
// than a partial order.  Vertex ids must be dense [0,V).
template <ngraph_like G> nmaybe<nvector<int>> ntoposort(const G& graph) {
    int vertices = ni::ngraph_vertices(graph);
    nvector<int> indegree(vertices, 0);
    for (int from = 0; from < vertices; ++from) {
        decltype(auto) adjacency = graph.neighbors(from);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            npre(0 <= to && to < vertices);
            npre(indegree[to] < INT_MAX);
            ++indegree[to];
        }
    }

    ndeque<int> queue;
    for (int vertex = 0; vertex < vertices; ++vertex)
        if (indegree[vertex] == 0)
            queue.pushr(vertex);

    nvector<int> order;
    order.reserve(vertices);
    while (!queue.empty()) {
        int from = queue.popl();
        order.push(from);
        decltype(auto) adjacency = graph.neighbors(from);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            if (--indegree[to] == 0)
                queue.pushr(to);
        }
    }
    return order.len() == vertices ? nmaybe<nvector<int>>(move(order)) : nmaybe<nvector<int>>{};
}

template <ngraph_like G> npartition nscc(const G& graph) {
    int vertices = ni::ngraph_vertices(graph);
    auto forward = vector<vector<int>>(size_t(vertices));
    auto reverse = vector<vector<int>>(size_t(vertices));
    for (int from = 0; from < vertices; ++from) {
        decltype(auto) adjacency = graph.neighbors(from);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            npre(0 <= to && to < vertices);
            npre(forward[from].size() < size_t(INT_MAX));
            npre(reverse[to].size() < size_t(INT_MAX));
            forward[from].push_back(to);
            reverse[to].push_back(from);
        }
    }

    vector<unsigned char> seen(size_t(vertices), false);
    vector<int> order;
    order.reserve(size_t(vertices));
    for (int start = 0; start < vertices; ++start) {
        if (seen[start])
            continue;
        seen[start] = true;
        vector<pair<int, int>> stack{{start, 0}};
        while (!stack.empty()) {
            int vertex = stack.back().first;
            int& position = stack.back().second;
            if (position < int(forward[vertex].size())) {
                int to = forward[vertex][position++];
                if (!seen[to]) {
                    seen[to] = true;
                    stack.push_back({to, 0});
                }
            } else {
                order.push_back(vertex);
                stack.pop_back();
            }
        }
    }

    nvector<int> component(vertices, npos);
    int count = 0;
    for (int position = vertices; position-- > 0;) {
        int start = order[position];
        if (component[start] != npos)
            continue;
        component[start] = count;
        vector<int> stack{start};
        while (!stack.empty()) {
            int vertex = stack.back();
            stack.pop_back();
            for (int to : reverse[vertex])
                if (component[to] == npos) {
                    component[to] = count;
                    stack.push_back(to);
                }
        }
        ++count;
    }
    return npartition(move(component));
}

template <ngraph_like G> nmaybe<nvector<int>> ntopo(const G& graph) {
    return ntoposort(graph);
}

template <ngraph_like G> nvector<int> ntopo(const G& graph, nvector<int> fallback) {
    auto result = ntoposort(graph);
    return result ? move(result.val()) : move(fallback);
}

template <ngraph_like G> npartition nscc_kosaraju(const G& graph) { return nscc(graph); }

template <ngraph_like G> npartition nscc_tarjan(const G& graph) {
    int vertices = ni::ngraph_vertices(graph);
    nvector<int> discovered(vertices, 0), low(vertices, 0), component(vertices, npos), stack;
    nvector<unsigned char> active(vertices, false);
    int timer = 0, components = 0;
    auto visit = [&](auto&& self, int from) -> void {
        discovered[from] = low[from] = ++timer;
        stack.push(from);
        active[from] = true;
        decltype(auto) adjacency = graph.neighbors(from);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            npre(0 <= to && to < vertices);
            if (!discovered[to]) {
                self(self, to);
                nchmin(low[from], low[to]);
            } else if (active[to]) {
                nchmin(low[from], discovered[to]);
            }
        }
        if (low[from] != discovered[from])
            return;
        for (;;) {
            int vertex = stack.pop();
            active[vertex] = false;
            component[vertex] = components;
            if (vertex == from)
                break;
        }
        ++components;
    };
    for (int vertex = 0; vertex < vertices; ++vertex)
        if (!discovered[vertex])
            visit(visit, vertex);
    return npartition(move(component));
}

namespace ni {
// Rooted-tree layout is only meaningful for a connected acyclic undirected graph.
// `require_symmetric` additionally checks that every adjacency arc has a reverse.
struct ntree_layout {
    vector<vector<int>> adjacency;
    nvector<int> parent, order;
};

template <ngraph_like G>
ntree_layout nbuild_tree_layout(const G& graph, int root, bool require_symmetric) {
    int vertices = ngraph_vertices(graph);
    npre(vertices > 0 && 0 <= root && root < vertices);
    ntree_layout result{vector<vector<int>>(size_t(vertices)), nvector<int>(vertices, npos), {}};

    for (int from = 0; from < vertices; ++from) {
        decltype(auto) edges = graph.neighbors(from);
        nfor(edge, edges) {
            int to = nedge_to(edge);
            npre(0 <= to && to < vertices && to != from);
            npre(result.adjacency[from].size() < size_t(INT_MAX));
            result.adjacency[from].push_back(to);
        }
    }

    result.order.reserve(vertices);
    result.parent[root] = root;
    result.order.push(root);
    for (int position = 0; position < result.order.len(); ++position) {
        int from = result.order[position];
        for (int to : result.adjacency[from])
            if (result.parent[to] == npos) {
                result.parent[to] = from;
                result.order.push(to);
            }
    }
    npre(result.order.len() == vertices);

    long long forward_arcs = 0, reverse_arcs = 0;
    for (int from = 0; from < vertices; ++from)
        for (int to : result.adjacency[from]) {
            if (result.parent[to] == from)
                ++forward_arcs;
            else if (from != root && to == result.parent[from])
                ++reverse_arcs;
            else
                npre(false);
        }
    npre(forward_arcs == vertices - 1LL);
    if (require_symmetric)
        npre(reverse_arcs == vertices - 1LL);
    else
        npre(reverse_arcs == 0 || reverse_arcs == vertices - 1LL);
    return result;
}
} // namespace ni

/**
 * Binary-lifting LCA index.  Input must be a connected undirected tree with symmetric
 * adjacency and a valid root.  Preprocessing is O(V log V), each query O(log V).
 */
class nlca {
    int vertices_ = 0;
    vector<vector<int>> ancestor_;
    nvector<int> depth_;

  public:
    nlca() = default;

    template <ngraph_like G> explicit nlca(const G& graph, int root = 0)
        : vertices_(ni::ngraph_vertices(graph)), depth_(vertices_, npos) {
        auto layout = ni::nbuild_tree_layout(graph, root, false);
        int levels = max(1, int(bit_width(unsigned(vertices_))));
        ancestor_.assign(size_t(levels), vector<int>(size_t(vertices_), root));
        depth_[root] = 0;
        ancestor_[0][root] = root;
        for (int position = 1; position < layout.order.len(); ++position) {
            int vertex = layout.order[position], parent = layout.parent[vertex];
            depth_[vertex] = depth_[parent] + 1;
            ancestor_[0][vertex] = parent;
        }
        for (int level = 1; level < levels; ++level)
            for (int vertex = 0; vertex < vertices_; ++vertex)
                ancestor_[level][vertex] = ancestor_[level - 1][ancestor_[level - 1][vertex]];
    }

    int len() const noexcept { return vertices_; }
    int depth(int vertex) const {
        npre(0 <= vertex && vertex < vertices_);
        return depth_[vertex];
    }

    int jump(int vertex, int steps) const {
        npre(0 <= vertex && vertex < vertices_ && steps >= 0);
        if (steps > depth_[vertex])
            return npos;
        for (int level = 0; steps; ++level, steps >>= 1)
            if (steps & 1)
                vertex = ancestor_[level][vertex];
        return vertex;
    }

    int operator()(int a, int b) const {
        npre(0 <= a && a < vertices_ && 0 <= b && b < vertices_);
        if (depth_[a] < depth_[b])
            swap(a, b);
        a = jump(a, depth_[a] - depth_[b]);
        if (a == b)
            return a;
        for (int level = int(ancestor_.size()); level-- > 0;)
            if (ancestor_[level][a] != ancestor_[level][b]) {
                a = ancestor_[level][a];
                b = ancestor_[level][b];
            }
        return ancestor_[0][a];
    }

    int distance(int a, int b) const {
        int common = (*this)(a, b);
        long long result = 1LL * depth_[a] + depth_[b] - 2LL * depth_[common];
        npre(0 <= result && result <= INT_MAX);
        return int(result);
    }

    int kth_on_path(int from, int to, int steps) const {
        npre(steps >= 0);
        int common = (*this)(from, to);
        int upward = depth_[from] - depth_[common];
        int downward = depth_[to] - depth_[common];
        long long length = 1LL * upward + downward;
        if (steps > length)
            return npos;
        return steps <= upward ? jump(from, steps) : jump(to, int(length - steps));
    }
};
