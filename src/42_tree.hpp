/**
 * Reroot/tree-path aggregation helper.  The graph must be a connected undirected tree;
 * Merge/Lift must preserve path direction and any noncommutative order chosen by the
 * caller.  All vertex ids are dense [0,V).
 */
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

struct nhld_segment {
    int l, r;
    bool rev;
};

/**
 * Heavy-light decomposition of a connected undirected tree.  path() emits half-open
 * base-array segments with a direction bit; noncommutative folds must honor that bit.
 */
class nhld {
    int vertices_ = 0;
    nvector<int> parent_, depth_, heavy_, head_, position_, inverse_, subtree_;

  public:
    nhld() = default;

    template <ngraph_like G> explicit nhld(const G& graph, int root = 0)
        : vertices_(ni::ngraph_vertices(graph)), parent_(vertices_), depth_(vertices_),
          heavy_(vertices_, npos), head_(vertices_), position_(vertices_),
          inverse_(vertices_), subtree_(vertices_, 1) {
        if (!vertices_) {
            npre(root == 0);
            return;
        }
        auto layout = ni::nbuild_tree_layout(graph, root, false);
        parent_ = layout.parent;
        depth_[root] = 0;
        for (int index = 1; index < layout.order.len(); ++index) {
            int vertex = layout.order[index];
            depth_[vertex] = depth_[parent_[vertex]] + 1;
        }
        for (int index = vertices_; index-- > 0;) {
            int vertex = layout.order[index];
            int best_size = 0;
            for (int to : layout.adjacency[vertex])
                if (parent_[to] == vertex) {
                    npre(subtree_[vertex] <= INT_MAX - subtree_[to]);
                    subtree_[vertex] += subtree_[to];
                    if (best_size < subtree_[to]) {
                        best_size = subtree_[to];
                        heavy_[vertex] = to;
                    }
                }
        }

        vector<pair<int, int>> pending{{root, root}};
        int timer = 0;
        while (!pending.empty()) {
            auto [start, chain_head] = pending.back();
            pending.pop_back();
            for (int vertex = start; vertex != npos; vertex = heavy_[vertex]) {
                head_[vertex] = chain_head;
                position_[vertex] = timer;
                inverse_[timer++] = vertex;
                for (int to : layout.adjacency[vertex])
                    if (parent_[to] == vertex && to != heavy_[vertex])
                        pending.push_back({to, to});
            }
        }
        npre(timer == vertices_);
    }

    int len() const noexcept { return vertices_; }
    bool empty() const noexcept { return vertices_ == 0; }
    bool same(int a, int b) const {
        return 0 <= a && a < vertices_ && 0 <= b && b < vertices_;
    }
    int parent(int vertex) const {
        npre(0 <= vertex && vertex < vertices_);
        return parent_[vertex];
    }
    int depth(int vertex) const {
        npre(0 <= vertex && vertex < vertices_);
        return depth_[vertex];
    }
    int position(int vertex) const {
        npre(0 <= vertex && vertex < vertices_);
        return position_[vertex];
    }
    int vertex(int position) const {
        npre(0 <= position && position < vertices_);
        return inverse_[position];
    }
    int lca(int a, int b, int fallback = npos) const {
        if (!same(a, b))
            return fallback;
        while (head_[a] != head_[b]) {
            if (depth_[head_[a]] < depth_[head_[b]])
                swap(a, b);
            a = parent_[head_[a]];
        }
        return depth_[a] < depth_[b] ? a : b;
    }
    nvector<nhld_segment> path(int a, int b, bool edge = false) const {
        nvector<nhld_segment> left, right;
        if (!same(a, b))
            return left;
        while (head_[a] != head_[b]) {
            if (depth_[head_[a]] >= depth_[head_[b]]) {
                left.push(nhld_segment{position_[head_[a]], position_[a] + 1, true});
                a = parent_[head_[a]];
            } else {
                right.push(nhld_segment{position_[head_[b]], position_[b] + 1, false});
                b = parent_[head_[b]];
            }
        }
        if (depth_[a] >= depth_[b]) {
            int first = position_[b] + int(edge);
            if (first <= position_[a])
                left.push(nhld_segment{first, position_[a] + 1, true});
        } else {
            int first = position_[a] + int(edge);
            if (first <= position_[b])
                right.push(nhld_segment{first, position_[b] + 1, false});
        }
        for (int index = right.len(); index-- > 0;)
            left.push(right[index]);
        return left;
    }
    pair<int, int> subtree(int vertex, bool edge = false) const {
        npre(0 <= vertex && vertex < vertices_);
        int left = position_[vertex] + int(edge);
        return {left, position_[vertex] + subtree_[vertex]};
    }
    template <class F> bool each(int a, int b, F visit, bool edge = false) const {
        if (!same(a, b))
            return false;
        nfor(segment, path(a, b, edge))
            invoke(visit, segment.l, segment.r, segment.rev);
        return true;
    }
};

// Binary-lifting LCA over a validated rooted tree.  Ancestor tables are immutable after
// construction; every query vertex belongs to the original [0,V) universe.
template <class W = int> class nlca_binary {
    int vertices_ = 0, levels_ = 0;
    vector<vector<int>> ancestor_;
    nvector<int> depth_, component_;
    nvector<W> distance_;

    template <class A, class B> static W add_distance(const A& a, const B& b) {
        if constexpr (is_integral_v<W>)
            return ni::nchecked_add(W(a), ni::nchecked_number<W>(b));
        else
            return W(a) + W(b);
    }

  public:
    nlca_binary() = default;

    template <ngraph_like G> explicit nlca_binary(const G& graph, int root = 0) {
        build(graph, root, [](const auto& edge) -> decltype(auto) { return nedge_weight(edge); });
    }
    template <ngraph_like G, class F> nlca_binary(const G& graph, int root, F weight) {
        build(graph, root, move(weight));
    }

    template <ngraph_like G, class F> void build(const G& graph, int root, F weight) {
        vertices_ = ni::ngraph_vertices(graph);
        levels_ = max(1, int(bit_width(unsigned(max(1, vertices_)))));
        ancestor_.assign(size_t(levels_), vector<int>(size_t(vertices_), 0));
        depth_ = nvector<int>(vertices_, npos);
        component_ = nvector<int>(vertices_, npos);
        distance_ = nvector<W>(vertices_, W{});
        if (!vertices_) {
            npre(root == 0);
            return;
        }
        npre(0 <= root && root < vertices_);
        nvector<int> starts;
        starts.reserve(vertices_);
        starts.push(root);
        for (int vertex = 0; vertex < vertices_; ++vertex)
            if (vertex != root)
                starts.push(vertex);

        nfor(start, starts) {
            if (depth_[start] != npos)
                continue;
            deque<int> queue;
            depth_[start] = 0;
            component_[start] = start;
            ancestor_[0][start] = start;
            queue.push_back(start);
            while (!queue.empty()) {
                int from = queue.front();
                queue.pop_front();
                decltype(auto) adjacency = graph.neighbors(from);
                nfor(edge, adjacency) {
                    int to = nedge_to(edge);
                    npre(0 <= to && to < vertices_);
                    if (depth_[to] != npos)
                        continue;
                    depth_[to] = depth_[from] + 1;
                    component_[to] = start;
                    ancestor_[0][to] = from;
                    distance_[to] = add_distance(distance_[from], invoke(weight, edge));
                    queue.push_back(to);
                }
            }
        }
        for (int level = 1; level < levels_; ++level)
            for (int vertex = 0; vertex < vertices_; ++vertex)
                ancestor_[level][vertex] = ancestor_[level - 1][ancestor_[level - 1][vertex]];
    }

    int len() const noexcept { return vertices_; }
    bool same(int a, int b) const {
        return 0 <= a && a < vertices_ && 0 <= b && b < vertices_ &&
               component_[a] == component_[b];
    }
    int depth(int vertex) const {
        npre(0 <= vertex && vertex < vertices_);
        return depth_[vertex];
    }
    int jump(int vertex, int steps, int fallback = npos) const {
        if (vertex < 0 || vertex >= vertices_ || steps < 0 || steps > depth_[vertex])
            return fallback;
        for (int level = 0; steps; ++level, steps >>= 1)
            if (steps & 1)
                vertex = ancestor_[level][vertex];
        return vertex;
    }
    int lca(int a, int b, int fallback = npos) const {
        if (!same(a, b))
            return fallback;
        if (depth_[a] < depth_[b])
            swap(a, b);
        a = jump(a, depth_[a] - depth_[b]);
        if (a == b)
            return a;
        for (int level = levels_; level-- > 0;)
            if (ancestor_[level][a] != ancestor_[level][b]) {
                a = ancestor_[level][a];
                b = ancestor_[level][b];
            }
        return ancestor_[0][a];
    }
    int operator()(int a, int b) const {
        int result = lca(a, b);
        npre(result != npos);
        return result;
    }
    W dist(int a, int b, W fallback) const {
        int common = lca(a, b);
        if (common == npos)
            return fallback;
        if constexpr (is_integral_v<W>) {
            __int128_t result = __int128_t(distance_[a]) + distance_[b] -
                                2 * __int128_t(distance_[common]);
            return ni::nchecked_integral_cast<W>(result);
        } else {
            return distance_[a] + distance_[b] - W{2} * distance_[common];
        }
    }
    W dist(int a, int b) const {
        npre(same(a, b));
        return dist(a, b, W{});
    }
    int kth(int from, int to, int steps, int fallback = npos) const {
        if (steps < 0)
            return fallback;
        int common = lca(from, to);
        if (common == npos)
            return fallback;
        int upward = depth_[from] - depth_[common];
        int downward = depth_[to] - depth_[common];
        if (steps > upward + downward)
            return fallback;
        return steps <= upward ? jump(from, steps) : jump(to, upward + downward - steps);
    }
};
