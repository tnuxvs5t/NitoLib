template <ngraph_like G> nmaybe<nvector<int>> ntoposort(const G& graph) {
    int vertices = graph.vertices();
    nvector<int> indegree(vertices, 0);
    for (int from = 0; from < vertices; ++from) {
        auto adjacency = graph.neighbors(from);
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
        auto adjacency = graph.neighbors(from);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            if (--indegree[to] == 0)
                queue.pushr(to);
        }
    }
    return order.len() == vertices ? nmaybe<nvector<int>>(move(order)) : nmaybe<nvector<int>>{};
}

template <ngraph_like G> npartition nscc(const G& graph) {
    int vertices = graph.vertices();
    auto forward = vector<vector<int>>(size_t(vertices));
    auto reverse = vector<vector<int>>(size_t(vertices));
    for (int from = 0; from < vertices; ++from) {
        auto adjacency = graph.neighbors(from);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            npre(0 <= to && to < vertices);
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

class nlca {
    int vertices_ = 0;
    vector<vector<int>> ancestor_;
    nvector<int> depth_;

  public:
    nlca() = default;

    template <ngraph_like G> explicit nlca(const G& graph, int root = 0)
        : vertices_(graph.vertices()), depth_(vertices_, npos) {
        npre(vertices_ > 0 && 0 <= root && root < vertices_);
        int levels = max(1, int(bit_width(unsigned(vertices_))));
        ancestor_.assign(size_t(levels), vector<int>(size_t(vertices_), root));
        ndeque<int> queue;
        depth_[root] = 0;
        ancestor_[0][root] = root;
        queue.pushr(root);
        int visited = 0;
        while (!queue.empty()) {
            int vertex = queue.popl();
            ++visited;
            auto adjacency = graph.neighbors(vertex);
            nfor(edge, adjacency) {
                int to = nedge_to(edge);
                npre(0 <= to && to < vertices_);
                if (depth_[to] == npos) {
                    depth_[to] = depth_[vertex] + 1;
                    ancestor_[0][to] = vertex;
                    queue.pushr(to);
                }
            }
        }
        npre(visited == vertices_);
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
        return depth_[a] + depth_[b] - 2 * depth_[common];
    }

    int kth_on_path(int from, int to, int steps) const {
        npre(steps >= 0);
        int common = (*this)(from, to);
        int upward = depth_[from] - depth_[common];
        int downward = depth_[to] - depth_[common];
        if (steps > upward + downward)
            return npos;
        return steps <= upward ? jump(from, steps) : jump(to, upward + downward - steps);
    }
};
