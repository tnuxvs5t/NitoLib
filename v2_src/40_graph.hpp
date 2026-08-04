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

template <class E>
    requires(!requires(const E& edge) { edge.weight; }) && requires(const E& edge) { edge.w; }
constexpr decltype(auto) nedge_weight(const E& edge) {
    return edge.w;
}

template <class W = int> struct nedge {
    int from, to, id;
    W w;
};

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
    graph.vertices();
    graph.neighbors(vertex);
    requires nenumerable<decltype(graph.neighbors(vertex))>;
} && (integral<remove_cvref_t<decltype(declval<const G&>().vertices())>> ||
      requires(const G& graph) {
          { nlen(graph.vertices()) } -> same_as<int>;
      }) && (!same_as<remove_cvref_t<decltype(declval<const G&>().vertices())>, bool>);

namespace ni {
template <ngraph_like G> constexpr int ngraph_vertices(const G& graph) {
    int vertices;
    if constexpr (integral<remove_cvref_t<decltype(graph.vertices())>>) {
        static_assert(!same_as<remove_cvref_t<decltype(graph.vertices())>, bool>);
        vertices = nchecked_int(graph.vertices());
    } else {
        vertices = nlen(graph.vertices());
    }
    npre(vertices >= 0);
    return vertices;
}

template <class G>
using ngraph_neighbor_t = decltype(
    nenumerate(declval<decltype(declval<const G&>().neighbors(0))>()).val());

template <class G>
using ngraph_weight_t = remove_cvref_t<decltype(nedge_weight(declval<ngraph_neighbor_t<G>>()))>;
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

template <class W = int> class ngraph_forward {
    int vertices_ = 0;
    vector<int> head_, to_, next_;
    vector<W> weight_;

    static size_t checked_vertices(int vertices) {
        npre(vertices >= 0);
        return size_t(vertices);
    }

    template <bool Constant> struct adjacency_cursor {
        using graph_type = conditional_t<Constant, const ngraph_forward, ngraph_forward>;
        using weight_reference = conditional_t<Constant, const W&, W&>;
        graph_type* graph;
        int from, edge, index = 0;

        bool ok() const { return edge != npos; }
        nedge<weight_reference> val() const {
            return {from, graph->to_[edge], edge, graph->weight_[edge]};
        }
        int idx() const { return index; }
        void next() {
            edge = graph->next_[edge];
            ++index;
        }
    };

    template <bool Constant> class adjacency_view {
        using graph_type = conditional_t<Constant, const ngraph_forward, ngraph_forward>;
        graph_type* graph_;
        int vertex_;

      public:
        adjacency_view(graph_type* graph, int vertex) : graph_(graph), vertex_(vertex) {}
        int len() const {
            int result = 0;
            for (int edge = graph_->head_[vertex_]; edge != npos; edge = graph_->next_[edge])
                ++result;
            return result;
        }
        bool empty() const { return graph_->head_[vertex_] == npos; }
        auto enumerate() const {
            return adjacency_cursor<Constant>{graph_, vertex_, graph_->head_[vertex_]};
        }
    };

    template <bool Constant> struct arcs_cursor {
        using graph_type = conditional_t<Constant, const ngraph_forward, ngraph_forward>;
        using weight_reference = conditional_t<Constant, const W&, W&>;
        graph_type* graph;
        int from = 0, edge = npos, index = 0;

        explicit arcs_cursor(graph_type* graph) : graph(graph) { seek(); }
        void seek() {
            while (from < graph->vertices_ && graph->head_[from] == npos)
                ++from;
            edge = from < graph->vertices_ ? graph->head_[from] : npos;
        }
        bool ok() const { return edge != npos; }
        nedge<weight_reference> val() const {
            return {from, graph->to_[edge], edge, graph->weight_[edge]};
        }
        int idx() const { return index; }
        void next() {
            edge = graph->next_[edge];
            ++index;
            if (edge == npos) {
                ++from;
                seek();
            }
        }
    };

    template <bool Constant> class arcs_view {
        using graph_type = conditional_t<Constant, const ngraph_forward, ngraph_forward>;
        graph_type* graph_;

      public:
        explicit arcs_view(graph_type* graph) : graph_(graph) {}
        int len() const { return graph_->edges(); }
        bool empty() const { return graph_->edges() == 0; }
        auto enumerate() const { return arcs_cursor<Constant>(graph_); }
    };

  public:
    using edge = nedge<W>;
    using view = adjacency_view<false>;
    using const_view = adjacency_view<true>;

    ngraph_forward() = default;
    explicit ngraph_forward(int vertices, int expected_edges = 0)
        : vertices_(vertices), head_(checked_vertices(vertices), npos) {
        reserve(expected_edges);
    }

    int len() const noexcept { return vertices_; }
    int edges() const {
        npre(to_.size() <= size_t(INT_MAX));
        return int(to_.size());
    }
    bool empty() const noexcept { return vertices_ == 0; }
    void reserve(int expected_edges) {
        npre(expected_edges >= 0);
        to_.reserve(size_t(expected_edges));
        next_.reserve(size_t(expected_edges));
        weight_.reserve(size_t(expected_edges));
    }
    void clear_edges() {
        fill(head_.begin(), head_.end(), npos);
        to_.clear();
        next_.clear();
        weight_.clear();
    }
    int add(int from, int to, W weight = W{1}) {
        npre(0 <= from && from < vertices_ && 0 <= to && to < vertices_);
        npre(to_.size() < size_t(INT_MAX));
        int id = edges();
        to_.push_back(to);
        next_.push_back(head_[from]);
        weight_.push_back(move(weight));
        head_[from] = id;
        return id;
    }
    pair<int, int> add2(int a, int b, W weight = W{1}) {
        int forward = add(a, b, weight);
        int backward = add(b, a, move(weight));
        return {forward, backward};
    }

    view neighbors(int vertex) {
        npre(0 <= vertex && vertex < vertices_);
        return {this, vertex};
    }
    const_view neighbors(int vertex) const {
        npre(0 <= vertex && vertex < vertices_);
        return {this, vertex};
    }
    view operator[](int vertex) & { return neighbors(vertex); }
    const_view operator[](int vertex) const& { return neighbors(vertex); }
    view operator[](int) && = delete;
    const_view operator[](int) const&& = delete;
    view from(int vertex) & { return neighbors(vertex); }
    const_view from(int vertex) const& { return neighbors(vertex); }
    view from(int) && = delete;
    const_view from(int) const&& = delete;

    int degree(int vertex) const { return neighbors(vertex).len(); }
    int find(int from, int to, int fallback = npos) const {
        npre(0 <= from && from < vertices_ && 0 <= to && to < vertices_);
        nfor(edge, neighbors(from))
            if (edge.to == to)
                return edge.id;
        return fallback;
    }
    bool has(int from, int to) const { return find(from, to) != npos; }
    W* weight(int edge) { return 0 <= edge && edge < edges() ? addressof(weight_[edge]) : nullptr; }
    const W* weight(int edge) const {
        return 0 <= edge && edge < edges() ? addressof(weight_[edge]) : nullptr;
    }
    W weight(int edge, W fallback) const {
        return 0 <= edge && edge < edges() ? weight_[edge] : move(fallback);
    }
    bool set(int edge, W weight) {
        if (edge < 0 || edge >= edges())
            return false;
        weight_[edge] = move(weight);
        return true;
    }

    auto vertices() const { return nrange(vertices_); }
    auto arcs() & { return arcs_view<false>(this); }
    auto arcs() const& { return arcs_view<true>(this); }
    auto arcs() && = delete;
    auto arcs() const&& = delete;

    ngraph_forward reverse() const {
        ngraph_forward result(vertices_, edges());
        nfor(edge, arcs())
            result.add(edge.to, edge.from, edge.w);
        return result;
    }
};

template <class W = int> class ngraph_csr {
    int vertices_ = 0;
    vector<int> offset_{0}, to_;
    vector<W> weight_;

    static size_t checked_vertices(int vertices) {
        npre(vertices >= 0);
        return size_t(vertices);
    }

    template <bool Constant> struct adjacency_cursor {
        using graph_type = conditional_t<Constant, const ngraph_csr, ngraph_csr>;
        using weight_reference = conditional_t<Constant, const W&, W&>;
        graph_type* graph;
        int from, edge, last, index = 0;
        bool ok() const { return edge < last; }
        nedge<weight_reference> val() const {
            return {from, graph->to_[edge], edge, graph->weight_[edge]};
        }
        int idx() const { return index; }
        void next() {
            ++edge;
            ++index;
        }
    };

    template <bool Constant> class adjacency_view {
        using graph_type = conditional_t<Constant, const ngraph_csr, ngraph_csr>;
        graph_type* graph_;
        int vertex_;

      public:
        adjacency_view(graph_type* graph, int vertex) : graph_(graph), vertex_(vertex) {}
        int len() const { return graph_->offset_[vertex_ + 1] - graph_->offset_[vertex_]; }
        bool empty() const { return len() == 0; }
        auto enumerate() const {
            return adjacency_cursor<Constant>{graph_, vertex_, graph_->offset_[vertex_],
                                              graph_->offset_[vertex_ + 1]};
        }
    };

    template <bool Constant> struct arcs_cursor {
        using graph_type = conditional_t<Constant, const ngraph_csr, ngraph_csr>;
        using weight_reference = conditional_t<Constant, const W&, W&>;
        graph_type* graph;
        mutable int from = 0;
        int edge = 0;

        bool ok() const { return edge < graph->edges(); }
        nedge<weight_reference> val() const {
            while (from + 1 < int(graph->offset_.size()) && graph->offset_[from + 1] <= edge)
                ++from;
            return {from, graph->to_[edge], edge, graph->weight_[edge]};
        }
        int idx() const { return edge; }
        void next() { ++edge; }
    };

    template <bool Constant> class arcs_view {
        using graph_type = conditional_t<Constant, const ngraph_csr, ngraph_csr>;
        graph_type* graph_;

      public:
        explicit arcs_view(graph_type* graph) : graph_(graph) {}
        int len() const { return graph_->edges(); }
        bool empty() const { return graph_->edges() == 0; }
        auto enumerate() const { return arcs_cursor<Constant>{graph_}; }
    };

  public:
    using edge = nedge<W>;
    using view = adjacency_view<false>;
    using const_view = adjacency_view<true>;

    ngraph_csr() = default;
    explicit ngraph_csr(int vertices)
        : vertices_(vertices), offset_(checked_vertices(vertices) + 1, 0) {}
    template <ngraph_like G> explicit ngraph_csr(const G& graph) { build(graph); }

    template <ngraph_like G> void build(const G& graph) {
        vertices_ = ni::ngraph_vertices(graph);
        offset_.assign(size_t(vertices_) + 1, 0);
        for (int vertex = 0; vertex < vertices_; ++vertex) {
            decltype(auto) adjacency = graph.neighbors(vertex);
            nfor(edge, adjacency) {
                int to = nedge_to(edge);
                npre(0 <= to && to < vertices_);
                npre(offset_[vertex + 1] < INT_MAX);
                ++offset_[vertex + 1];
            }
            npre(offset_[vertex] <= INT_MAX - offset_[vertex + 1]);
            offset_[vertex + 1] += offset_[vertex];
        }
        to_.clear();
        weight_.clear();
        to_.reserve(size_t(offset_.back()));
        weight_.reserve(size_t(offset_.back()));
        for (int vertex = 0; vertex < vertices_; ++vertex) {
            decltype(auto) adjacency = graph.neighbors(vertex);
            nfor(edge, adjacency) {
                to_.push_back(nedge_to(edge));
                weight_.push_back(W(nedge_weight(edge)));
            }
        }
    }

    int len() const noexcept { return vertices_; }
    int edges() const {
        npre(to_.size() <= size_t(INT_MAX));
        return int(to_.size());
    }
    bool empty() const noexcept { return vertices_ == 0; }
    view neighbors(int vertex) {
        npre(0 <= vertex && vertex < vertices_);
        return {this, vertex};
    }
    const_view neighbors(int vertex) const {
        npre(0 <= vertex && vertex < vertices_);
        return {this, vertex};
    }
    view operator[](int vertex) & { return neighbors(vertex); }
    const_view operator[](int vertex) const& { return neighbors(vertex); }
    view operator[](int) && = delete;
    const_view operator[](int) const&& = delete;
    view from(int vertex) & { return neighbors(vertex); }
    const_view from(int vertex) const& { return neighbors(vertex); }
    view from(int) && = delete;
    const_view from(int) const&& = delete;

    int degree(int vertex) const { return neighbors(vertex).len(); }
    int find(int from, int to, int fallback = npos) const {
        npre(0 <= from && from < vertices_ && 0 <= to && to < vertices_);
        nfor(edge, neighbors(from))
            if (edge.to == to)
                return edge.id;
        return fallback;
    }
    bool has(int from, int to) const { return find(from, to) != npos; }
    W* weight(int edge) { return 0 <= edge && edge < edges() ? addressof(weight_[edge]) : nullptr; }
    const W* weight(int edge) const {
        return 0 <= edge && edge < edges() ? addressof(weight_[edge]) : nullptr;
    }
    W weight(int edge, W fallback) const {
        return 0 <= edge && edge < edges() ? weight_[edge] : move(fallback);
    }
    bool set(int edge, W weight) {
        if (edge < 0 || edge >= edges())
            return false;
        weight_[edge] = move(weight);
        return true;
    }

    auto vertices() const { return nrange(vertices_); }
    auto arcs() & { return arcs_view<false>(this); }
    auto arcs() const& { return arcs_view<true>(this); }
    auto arcs() && = delete;
    auto arcs() const&& = delete;

    ngraph_csr reverse() const {
        ngraph_forward<W> reversed(vertices_, edges());
        nfor(edge, arcs())
            reversed.add(edge.to, edge.from, edge.w);
        return ngraph_csr(reversed);
    }
};

template <ngraph_like G> ngraph_csr(const G&) -> ngraph_csr<ni::ngraph_weight_t<G>>;

template <class W = int> using ngraph = ngraph_forward<W>;

template <ngraph_like G> auto nvertices(const G& graph) {
    return nrange(ni::ngraph_vertices(graph));
}

template <class G> class ngraph_arcs_view {
    G* graph_;
    using adjacency_type = decltype(declval<G&>().neighbors(0));
    using inner_cursor = nenumerator_t<adjacency_type>;

    struct cursor {
        G* graph;
        int vertex = 0, index = 0;
        optional<inner_cursor> inner;

        explicit cursor(G* graph) : graph(graph) { seek(); }
        void seek() {
            int vertices = ni::ngraph_vertices(*graph);
            while (vertex < vertices) {
                // Preserve the value category returned by neighbors(): a temporary adjacency
                // is owned by its enumerator, while a reference remains borrowed from graph.
                inner.emplace(nenumerate(graph->neighbors(vertex)));
                if (inner->ok())
                    return;
                ++vertex;
            }
            inner.reset();
        }
        bool ok() const { return inner.has_value(); }
        auto val() {
            decltype(auto) raw = inner->val();
            using weight_reference = decltype(nedge_weight(raw));
            int id;
            if constexpr (requires { raw.id; })
                id = raw.id;
            else
                id = index;
            return nedge<weight_reference>{vertex, nedge_to(raw), id, nedge_weight(raw)};
        }
        int idx() const { return index; }
        void next() {
            inner->next();
            ++index;
            if (!inner->ok()) {
                ++vertex;
                seek();
            }
        }
    };

  public:
    explicit ngraph_arcs_view(G& graph) : graph_(addressof(graph)) {}
    int len() const {
        int result = 0;
        for (int vertex = 0; vertex < ni::ngraph_vertices(*graph_); ++vertex) {
            decltype(auto) adjacency = graph_->neighbors(vertex);
            nfor(edge, adjacency) {
                (void)edge;
                npre(result < INT_MAX);
                ++result;
            }
        }
        return result;
    }
    bool empty() const {
        for (int vertex = 0; vertex < ni::ngraph_vertices(*graph_); ++vertex) {
            decltype(auto) adjacency = graph_->neighbors(vertex);
            auto enumeration = nenumerate(adjacency);
            if (enumeration.ok())
                return false;
        }
        return true;
    }
    auto enumerate() const { return cursor(graph_); }
};

template <ngraph_like G> auto narcs(G& graph) { return ngraph_arcs_view<G>(graph); }
template <ngraph_like G> auto narcs(const G& graph) { return ngraph_arcs_view<const G>(graph); }
template <ngraph_like G> auto narcs(G&&) = delete;

template <class D> struct npath_result {
    nvector<D> d;
    nvector<int> p;
    D bad{};

    int len() const noexcept { return d.len(); }
    bool reach(int vertex) const {
        return 0 <= vertex && vertex < len() && vertex < p.len() && p[vertex] != npos;
    }
    D dist(int vertex, D fallback) const { return reach(vertex) ? d[vertex] : move(fallback); }
    const D& operator[](int vertex) const { return d[vertex]; }
    nvector<int> path(int vertex) const {
        nvector<int> result;
        if (!reach(vertex))
            return result;
        for (;;) {
            result.push(vertex);
            if (p[vertex] == vertex)
                break;
            vertex = p[vertex];
        }
        nreverse_inplace(result);
        return result;
    }
};

template <ngraph_like G> npath_result<int> nbfs_path(const G& graph, int source) {
    int vertices = ni::ngraph_vertices(graph);
    npre(0 <= source && source < vertices);
    npath_result<int> result{nvector<int>(vertices, npos), nvector<int>(vertices, npos), npos};
    deque<int> queue;
    result.d[source] = 0;
    result.p[source] = source;
    queue.push_back(source);
    while (!queue.empty()) {
        int from = queue.front();
        queue.pop_front();
        decltype(auto) adjacency = graph.neighbors(from);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            npre(0 <= to && to < vertices);
            if (result.p[to] == npos) {
                result.d[to] = result.d[from] + 1;
                result.p[to] = from;
                queue.push_back(to);
            }
        }
    }
    return result;
}

template <class D = long long, ngraph_like G>
    requires is_arithmetic_v<D> && (!same_as<remove_cv_t<D>, bool>)
npath_result<D> ndijkstra_path(const G& graph, int source, D infinity = nmin<D>{}.id()) {
    int vertices = ni::ngraph_vertices(graph);
    npre(0 <= source && source < vertices && D{} <= infinity);
    npath_result<D> result{nvector<D>(vertices, infinity), nvector<int>(vertices, npos), infinity};
    using state = pair<D, int>;
    priority_queue<state, vector<state>, greater<state>> queue;
    result.d[source] = D{};
    result.p[source] = source;
    queue.push({D{}, source});
    while (!queue.empty()) {
        auto [distance, from] = queue.top();
        queue.pop();
        if (distance != result.d[from])
            continue;
        decltype(auto) adjacency = graph.neighbors(from);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            D weight = ni::nchecked_number<D>(nedge_weight(edge));
            npre(0 <= to && to < vertices && !(weight < D{}));
            if (distance <= infinity && weight <= infinity - distance) {
                D candidate = distance + weight;
                if (candidate < result.d[to]) {
                    result.d[to] = candidate;
                    result.p[to] = from;
                    queue.push({candidate, to});
                }
            }
        }
    }
    return result;
}

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
