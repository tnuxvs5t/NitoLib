template <class W = int> struct narc {
    int to;
    W weight;
    friend bool operator==(const narc&, const narc&) = default;
};

template <integral E> constexpr int nedge_to(E vertex) { return ni::nchecked_int(vertex); }
template <class E>
    requires requires(const E& edge) {
        requires integral<remove_cvref_t<decltype(edge.to)>>;
    }
constexpr int nedge_to(const E& edge) {
    return ni::nchecked_int(edge.to);
}

template <integral E> constexpr int nedge_weight(E) { return 1; }
template <class E>
    requires requires(const E& edge) { edge.weight; }
constexpr decltype(auto) nedge_weight(const E& edge) {
    return edge.weight;
}

template <class F> class ngraph_view {
    int vertices_ = 0;
    [[no_unique_address]] F adjacency_;

  public:
    ngraph_view(int vertices, F adjacency) : vertices_(vertices), adjacency_(move(adjacency)) {
        npre(vertices >= 0);
    }
    int vertices() const noexcept { return vertices_; }
    decltype(auto) neighbors(int vertex) const {
        npre(0 <= vertex && vertex < vertices_);
        return invoke(adjacency_, vertex);
    }
};

template <class F> ngraph_view(int, F) -> ngraph_view<F>;

template <class G>
concept ngraph_like = requires(const G& graph, int vertex) {
    requires integral<remove_cvref_t<decltype(graph.vertices())>>;
    requires(!same_as<remove_cvref_t<decltype(graph.vertices())>, bool>);
    graph.neighbors(vertex);
    requires nenumerable<decltype(graph.neighbors(vertex))>;
};

namespace ni {
template <ngraph_like G> constexpr int ngraph_vertices(const G& graph) {
    int vertices = nchecked_int(graph.vertices());
    npre(vertices >= 0);
    return vertices;
}
} // namespace ni

template <class W = int> class ngraph_list {
    vector<vector<narc<W>>> adjacency_;
    int arcs_ = 0;

    static size_t checked_vertices(int vertices) {
        npre(vertices >= 0);
        return size_t(vertices);
    }

  public:
    explicit ngraph_list(int vertices = 0) : adjacency_(checked_vertices(vertices)) {}

    int vertices() const noexcept { return int(adjacency_.size()); }
    int arcs() const noexcept { return arcs_; }

    void add(int from, int to, W weight = W{1}) {
        npre(0 <= from && from < vertices() && 0 <= to && to < vertices());
        npre(arcs_ < INT_MAX);
        npre(adjacency_[from].size() < size_t(INT_MAX));
        adjacency_[from].push_back({to, move(weight)});
        ++arcs_;
    }
    void add2(int a, int b, const W& weight = W{1}) {
        add(a, b, weight);
        add(b, a, weight);
    }

    nspan<const narc<W>> neighbors(int vertex) const {
        npre(0 <= vertex && vertex < vertices());
        const auto& edges = adjacency_[vertex];
        npre(edges.size() <= size_t(INT_MAX));
        return {edges.data(), int(edges.size())};
    }
};

template <ngraph_like G> nvector<int> nbfs(const G& graph, int source) {
    int vertices = ni::ngraph_vertices(graph);
    npre(0 <= source && source < vertices);
    nvector<int> distance(vertices, npos);
    ndeque<int> queue;
    distance[source] = 0;
    queue.pushr(source);
    while (!queue.empty()) {
        int vertex = queue.popl();
        decltype(auto) adjacency = graph.neighbors(vertex);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            npre(0 <= to && to < vertices);
            if (distance[to] == npos) {
                distance[to] = distance[vertex] + 1;
                queue.pushr(to);
            }
        }
    }
    return distance;
}

template <class D = long long, ngraph_like G>
    requires is_arithmetic_v<D> && (!same_as<remove_cv_t<D>, bool>)
nvector<D> ndijkstra(const G& graph, int source, D infinity = nmin<D>{}.id()) {
    int vertices = ni::ngraph_vertices(graph);
    npre(0 <= source && source < vertices);
    npre(D{} <= infinity);
    nvector<D> distance(vertices, infinity);
    using state = pair<D, int>;
    priority_queue<state, vector<state>, greater<state>> queue;
    distance[source] = D{};
    queue.push({D{}, source});

    while (!queue.empty()) {
        auto [current, vertex] = queue.top();
        queue.pop();
        if (current != distance[vertex])
            continue;
        decltype(auto) adjacency = graph.neighbors(vertex);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            D weight = ni::nchecked_number<D>(nedge_weight(edge));
            npre(0 <= to && to < vertices);
            npre(!(weight < D{}));
            if (current <= infinity && weight <= infinity - current) {
                D candidate = current + weight;
                if (candidate < distance[to]) {
                    distance[to] = candidate;
                    queue.push({candidate, to});
                }
            }
        }
    }
    return distance;
}
