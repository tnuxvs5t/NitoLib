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

namespace ni {
// This is an additive forest builder.  The old nbuild_tree_layout() deliberately
// remains the connected-tree checker used by the original dense APIs.  A forest
// component is rooted at `root` for the component containing it; other components
// infer their outward root from a unique zero-indegree vertex when possible, and
// otherwise use the smallest vertex.  The raw graph must describe every tree edge
// exactly once in the parent direction, plus either no reverse arcs or exactly one
// reverse arc per edge.  `require_symmetric` requires the latter in every component.
// A directed component and a symmetric component may coexist when it is false, but
// one component may not mix the two representations.  The returned adjacency is a
// canonical symmetric projection: missing reverse arcs are appended after the raw
// order, so old path/reroot code can consume the owner without a second graph view.
struct nforest_layout {
    vector<vector<int>> adjacency;
    nvector<int> parent, component, depth, order, roots;
};

template <ngraph_like G>
nforest_layout nbuild_forest_layout(const G& graph, int root, bool require_symmetric = false) {
    int vertices = ngraph_vertices(graph);
    npre(vertices == 0 ? root == 0 : 0 <= root && root < vertices);

    nforest_layout result;
    result.adjacency.resize(size_t(vertices));
    result.parent = nvector<int>(vertices, npos);
    result.component = nvector<int>(vertices, npos);
    result.depth = nvector<int>(vertices, npos);
    if (!vertices)
        return result;

    vector<vector<int>> raw, undirected;
    raw.resize(size_t(vertices));
    undirected.resize(size_t(vertices));
    vector<int> incoming(size_t(vertices), 0);
    for (int from = 0; from < vertices; ++from) {
        decltype(auto) edges = graph.neighbors(from);
        nfor(edge, edges) {
            int to = nedge_to(edge);
            npre(0 <= to && to < vertices && to != from);
            npre(raw[size_t(from)].size() < size_t(INT_MAX));
            npre(undirected[size_t(from)].size() < size_t(INT_MAX));
            npre(undirected[size_t(to)].size() < size_t(INT_MAX));
            npre(incoming[size_t(to)] < INT_MAX);
            raw[size_t(from)].push_back(to);
            undirected[size_t(from)].push_back(to);
            undirected[size_t(to)].push_back(from);
            ++incoming[size_t(to)];
        }
    }

    vector<int> raw_component(size_t(vertices), npos);
    vector<vector<int>> members;
    for (int start = 0; start < vertices; ++start) {
        if (raw_component[size_t(start)] != npos)
            continue;
        npre(members.size() < size_t(INT_MAX));
        int component = int(members.size());
        members.emplace_back();
        deque<int> queue;
        queue.push_back(start);
        raw_component[size_t(start)] = component;
        while (!queue.empty()) {
            int from = queue.front();
            queue.pop_front();
            members.back().push_back(from);
            for (int to : undirected[size_t(from)])
                if (raw_component[size_t(to)] == npos) {
                    raw_component[size_t(to)] = component;
                    queue.push_back(to);
                }
        }
    }

    int root_component = raw_component[size_t(root)];
    vector<int> component_order;
    component_order.reserve(members.size());
    component_order.push_back(root_component);
    for (int component = 0; component < int(members.size()); ++component)
        if (component != root_component)
            component_order.push_back(component);

    vector<int> chosen_root(members.size(), npos);
    chosen_root[size_t(root_component)] = root;
    for (int component = 0; component < int(members.size()); ++component) {
        if (component == root_component)
            continue;
        int candidate = npos, candidates = 0;
        for (int vertex : members[size_t(component)])
            if (incoming[size_t(vertex)] == 0) {
                candidate = vertex;
                ++candidates;
            }
        if (candidates == 1)
            chosen_root[size_t(component)] = candidate;
        else
            chosen_root[size_t(component)] = *min_element(members[size_t(component)].begin(),
                                                           members[size_t(component)].end());
    }

    result.roots.reserve(int(members.size()));
    result.order.reserve(vertices);
    for (int rank = 0; rank < int(component_order.size()); ++rank) {
        int component = component_order[size_t(rank)];
        int start = chosen_root[size_t(component)];
        result.roots.push(start);
        result.parent[start] = start;
        result.component[start] = rank;
        result.depth[start] = 0;
        result.order.push(start);
        int begin = result.order.len() - 1;
        for (int position = begin; position < result.order.len(); ++position) {
            int from = result.order[position];
            for (int to : undirected[size_t(from)])
                if (result.parent[to] == npos) {
                    result.parent[to] = from;
                    result.component[to] = rank;
                    npre(result.depth[from] < INT_MAX);
                    result.depth[to] = result.depth[from] + 1;
                    result.order.push(to);
                }
        }
        npre(result.order.len() > begin);
    }

    vector<int> forward(size_t(vertices), 0), reverse(size_t(vertices), 0);
    for (int from = 0; from < vertices; ++from)
        for (int to : raw[size_t(from)]) {
            if (result.parent[to] == from) {
                npre(forward[size_t(to)] < INT_MAX);
                ++forward[size_t(to)];
            } else if (result.parent[from] == to) {
                npre(reverse[size_t(from)] < INT_MAX);
                ++reverse[size_t(from)];
            } else {
                // A non-parent edge is a cycle, a chord, or a connection that the
                // undirected BFS did not place in this rooted forest.
                npre(false);
            }
        }

    for (int vertex = 0; vertex < vertices; ++vertex) {
        if (result.parent[vertex] == vertex) {
            npre(forward[size_t(vertex)] == 0 && reverse[size_t(vertex)] == 0);
            continue;
        }
        npre(forward[size_t(vertex)] == 1);
        npre(reverse[size_t(vertex)] <= 1);
    }

    for (int rank = 0; rank < int(component_order.size()); ++rank) {
        int component = component_order[size_t(rank)];
        bool has_edge = false, has_reverse = false, has_missing_reverse = false;
        for (int vertex : members[size_t(component)]) {
            if (result.parent[vertex] == vertex)
                continue;
            has_edge = true;
            has_reverse |= reverse[size_t(vertex)] == 1;
            has_missing_reverse |= reverse[size_t(vertex)] == 0;
            if (require_symmetric)
                npre(reverse[size_t(vertex)] == 1);
        }
        if (!require_symmetric && has_edge)
            npre(!(has_reverse && has_missing_reverse));
    }

    result.adjacency = raw;
    for (int vertex = 0; vertex < vertices; ++vertex) {
        if (result.parent[vertex] == vertex || reverse[size_t(vertex)] != 0)
            continue;
        int parent = result.parent[vertex];
        npre(result.adjacency[size_t(vertex)].size() < size_t(INT_MAX));
        result.adjacency[size_t(vertex)].push_back(parent);
    }
    npre(result.order.len() == vertices);
    return result;
}
} // namespace ni

/**
 * Resource-backed rooted forest projection.  `topology_` owns vertex and incidence
 * records in the same nnode_domain used by the graph module; this class adds only
 * rooted metadata and a dense child/order projection.  The public `node_view` is the
 * existing nnode_view supplied by that topology, not a graph/tree-specific view.
 *
 * A parent array uses `parent[v] == v` for a component root.  A graph constructor
 * accepts a strict forest in the same parent-direction/symmetric forms as the helper
 * above, then materializes a symmetric resource topology.  The owner is structurally
 * immutable in this stage; vertex payload edits are current, while all views borrowed
 * from the topology or child/order projections end at owner destruction or the future
 * topology transaction that rebuilds the projection.  Construction is O(V+E), all
 * scalar metadata queries are O(1), and `children(v)` is O(1) to create and enumerate.
 */
template <class V = monostate> class nrooted_forest {
    using topology_type = ngraph_topology<int, V>;

    topology_type topology_;
    nvector<int> parent_, component_, depth_, order_, position_, subtree_, roots_;
    nvector<int> child_begin_, child_end_, child_order_;

    void install(const ni::nforest_layout& layout) {
        int vertices = int(layout.adjacency.size());
        npre(topology_.len() == vertices && topology_.edges() == 0);

        long long arcs = 0;
        for (const auto& adjacency : layout.adjacency) {
            npre(adjacency.size() <= size_t(INT_MAX));
            arcs += adjacency.size();
        }
        npre(arcs <= INT_MAX);
        topology_.reserve(int(arcs));
        for (int from = 0; from < vertices; ++from)
            for (int to : layout.adjacency[size_t(from)])
                topology_.add(from, to);

        parent_ = layout.parent;
        component_ = layout.component;
        depth_ = layout.depth;
        order_ = layout.order;
        roots_ = layout.roots;
        position_ = nvector<int>(vertices, npos);
        for (int position = 0; position < order_.len(); ++position)
            position_[order_[position]] = position;

        subtree_ = nvector<int>(vertices, 1);
        for (int position = order_.len(); position-- > 0;) {
            int vertex = order_[position];
            if (parent_[vertex] == vertex)
                continue;
            npre(subtree_[parent_[vertex]] <= INT_MAX - subtree_[vertex]);
            subtree_[parent_[vertex]] += subtree_[vertex];
        }

        child_begin_ = nvector<int>(vertices, 0);
        child_end_ = nvector<int>(vertices, 0);
        child_order_.reserve(max(0, vertices - roots_.len()));
        for (int vertex = 0; vertex < vertices; ++vertex) {
            child_begin_[vertex] = child_order_.len();
            for (int to : layout.adjacency[size_t(vertex)])
                if (parent_[to] == vertex)
                    child_order_.push(to);
            child_end_[vertex] = child_order_.len();
        }
    }

    static ni::nforest_layout from_parent(const nvector<int>& parent) {
        int vertices = parent.len();
        ni::nforest_layout result;
        result.adjacency.resize(size_t(vertices));
        result.parent = parent;
        result.component = nvector<int>(vertices, npos);
        result.depth = nvector<int>(vertices, npos);
        vector<vector<int>> children;
        children.resize(size_t(vertices));
        vector<int> roots;
        for (int vertex = 0; vertex < vertices; ++vertex) {
            int ancestor = parent[vertex];
            npre(0 <= ancestor && ancestor < vertices);
            if (ancestor == vertex)
                roots.push_back(vertex);
            else {
                npre(children[size_t(ancestor)].size() < size_t(INT_MAX));
                children[size_t(ancestor)].push_back(vertex);
            }
        }
        if (vertices)
            npre(!roots.empty());

        result.roots.reserve(int(roots.size()));
        result.order.reserve(vertices);
        for (int component = 0; component < int(roots.size()); ++component) {
            int root = roots[size_t(component)];
            result.roots.push(root);
            result.component[root] = component;
            result.depth[root] = 0;
            result.order.push(root);
            int begin = result.order.len() - 1;
            for (int position = begin; position < result.order.len(); ++position) {
                int from = result.order[position];
                for (int to : children[size_t(from)]) {
                    npre(result.component[to] == npos);
                    result.component[to] = component;
                    npre(result.depth[from] < INT_MAX);
                    result.depth[to] = result.depth[from] + 1;
                    result.order.push(to);
                }
            }
        }
        npre(result.order.len() == vertices);
        for (int from = 0; from < vertices; ++from)
            for (int to : children[size_t(from)]) {
                npre(result.adjacency[size_t(from)].size() < size_t(INT_MAX));
                npre(result.adjacency[size_t(to)].size() < size_t(INT_MAX));
                result.adjacency[size_t(from)].push_back(to);
                result.adjacency[size_t(to)].push_back(from);
            }
        return result;
    }

    void install_isolated() {
        nvector<int> parent(topology_.len());
        for (int vertex = 0; vertex < parent.len(); ++vertex)
            parent[vertex] = vertex;
        install(from_parent(parent));
    }

  public:
    using value_type = typename topology_type::value_type;
    using info_type = typename topology_type::info_type;
    using domain_type = typename topology_type::domain_type;
    using node_view = typename topology_type::node_view;

    nrooted_forest() = default;
    explicit nrooted_forest(int vertices) : topology_(vertices) {
        npre(vertices >= 0);
        install_isolated();
    }
    explicit nrooted_forest(domain_type domain, int vertices = 0)
        : topology_(move(domain), vertices) {
        install_isolated();
    }
    explicit nrooted_forest(const nvector<int>& parent) : topology_(parent.len()) {
        install(from_parent(parent));
    }
    nrooted_forest(domain_type domain, const nvector<int>& parent)
        : topology_(move(domain), parent.len()) {
        install(from_parent(parent));
    }

    template <ngraph_like G>
    explicit nrooted_forest(const G& graph, int root = 0, bool require_symmetric = false)
        : topology_(ni::ngraph_vertices(graph)) {
        install(ni::nbuild_forest_layout(graph, root, require_symmetric));
    }

    int len() const noexcept { return topology_.len(); }
    int vertices() const noexcept { return len(); }
    int edges() const noexcept { return topology_.edges() / 2; }
    bool empty() const noexcept { return !len(); }
    int components() const noexcept { return roots_.len(); }
    bool connected() const noexcept { return components() <= 1; }
    auto vertex_ids() const { return nrange(len()); }
    auto roots() const { return nview(roots_.data(), roots_.len()); }
    auto order() const { return nview(order_.data(), order_.len()); }

    domain_type domain() const { return topology_.domain(); }
    bool same_domain(const nrooted_forest& other) const noexcept {
        return topology_.same_domain(other.topology_);
    }

    node_view vertex_node(int vertex) const {
        npre(0 <= vertex && vertex < len());
        return topology_.vertex_node(vertex);
    }
    node_view node(int vertex) const { return vertex_node(vertex); }
    V& vertex_value(int vertex) { return topology_.vertex_value(vertex); }
    const V& vertex_value(int vertex) const { return topology_.vertex_value(vertex); }
    void set_vertex(int vertex, V value) { topology_.set_vertex(vertex, move(value)); }

    auto neighbors(int vertex) const { return topology_.neighbors(vertex); }
    auto arcs() const& { return topology_.arcs(); }

    int degree(int vertex) const { return topology_.degree(vertex); }
    auto children(int vertex) const {
        npre(0 <= vertex && vertex < len());
        int left = child_begin_[vertex], right = child_end_[vertex];
        const int* data = child_order_.data();
        if (data)
            data += left;
        return nview(data, right - left);
    }
    int child_count(int vertex) const { return children(vertex).len(); }

    bool same(int left, int right) const noexcept {
        return 0 <= left && left < len() && 0 <= right && right < len() &&
               component_[left] == component_[right];
    }
    int parent(int vertex) const {
        npre(0 <= vertex && vertex < len());
        return parent_[vertex];
    }
    int component(int vertex) const {
        npre(0 <= vertex && vertex < len());
        return component_[vertex];
    }
    int root(int vertex) const {
        npre(0 <= vertex && vertex < len());
        return roots_[component_[vertex]];
    }
    int depth(int vertex) const {
        npre(0 <= vertex && vertex < len());
        return depth_[vertex];
    }
    int position(int vertex) const {
        npre(0 <= vertex && vertex < len());
        return position_[vertex];
    }
    int vertex(int position) const {
        npre(0 <= position && position < order_.len());
        return order_[position];
    }
    int subtree_size(int vertex) const {
        npre(0 <= vertex && vertex < len());
        return subtree_[vertex];
    }
    node_view parent_node(int vertex) const {
        int ancestor = parent(vertex);
        return ancestor == vertex ? node_view{} : vertex_node(ancestor);
    }

    ni::ntree_layout layout() const {
        ni::ntree_layout result{vector<vector<int>>(size_t(len())), parent_, order_};
        for (int vertex = 0; vertex < len(); ++vertex) {
            auto& adjacency = result.adjacency[size_t(vertex)];
            npre(adjacency.capacity() <= size_t(INT_MAX));
            nfor(edge, topology_.neighbors(vertex)) {
                npre(adjacency.size() < size_t(INT_MAX));
                adjacency.push_back(edge.to);
            }
        }
        return result;
    }
};

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
