// Directed weighted edge record.  `to` must be a vertex id in the owning graph; W is
// interpreted by the selected algorithm (Dijkstra additionally requires W >= 0).
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

// Undirected edge record used by MST/flow helpers.  Endpoints must belong to the graph;
// parallel edges are allowed and self-loops are algorithm-specific.
template <class W = int> struct nedge {
    int from, to, id;
    W w;
};

template <class T> constexpr T ncapadd(T left, T right, T infinity = ninf<T>) {
    if constexpr (integral<T> && sizeof(T) <= sizeof(uint64_t)) {
        using W = conditional_t<signed_integral<T>, __int128_t, __uint128_t>;
        W sum = W(left) + W(right);
        if (sum > W(infinity))
            return infinity;
        if constexpr (signed_integral<T>)
            if (sum < W(nninf<T>))
                return nninf<T>;
        return T(sum);
    } else {
        return left + right;
    }
}

/**
 * Borrowed graph capability view.  F(vertex) returns an enumerable neighbor range whose
 * edge objects expose a checked integer destination and optional weight.  The source
 * owner and captured adjacency must outlive this view; it never owns vertices.
 */
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

template <class G> using ngraph_edge_t = ngraph_neighbor_t<G>;
} // namespace ni

// Owning adjacency-list graph.  Algorithms assume every neighbor destination is in
// [0,vertices()); insertion may invalidate borrowed neighbor views.
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

    nview<const narc<W>> neighbors(int vertex) const {
        npre(0 <= vertex && vertex < vertices());
        const auto& edges = adjacency_[vertex];
        npre(edges.size() <= size_t(INT_MAX));
        return {edges.data(), int(edges.size())};
    }
};

enum class ngraph_node_kind : unsigned char { vertex, arc };

/**
 * Resource record used by `ngraph_topology`.  Vertex and arc records deliberately
 * share one `nnode_domain` so a node view can identify either kind without a second
 * graph-specific view type.  `head/tail/degree` belong to a vertex; `from/to/next/prev`
 * belong to an arc; link fields are resource handles, not dense public ids.  They are
 * exposed only through a const `nnode_view` and must be changed by the owner methods.
 */
template <class W = int, class V = monostate> struct ngraph_node_record {
    ngraph_node_kind kind;
    int id;
    int from = npos, to = npos;
    int next = 0, prev = 0;
    int head = 0, tail = 0, degree = 0;
    W weight{};
    optional<V> payload;

    ngraph_node_record(ngraph_node_kind kind, int id, V payload)
        : kind(kind), id(id), payload(move(payload)) {}
    ngraph_node_record(ngraph_node_kind kind, int id, int from, int to, W weight)
        : kind(kind), id(id), from(from), to(to), weight(move(weight)) {}
};

/**
 * Resource-backed owning graph topology.  Dense vertex ids and public arc ids are
 * kept as stable owner-side maps, while records live in one reusable node domain.
 * The first version intentionally exposes directed append/erase/rewire operations;
 * all of them publish one shared epoch after restoring the adjacency-chain and id-map
 * invariants.  `neighbors()` and `arcs()` are borrowed enumerable projections and
 * must not be retained across structural mutation.  Expected costs are O(1) for
 * add/erase/rewire (apart from resource allocation), O(1) for degree/weight access,
 * and O(outdegree) for find and adjacency enumeration.
 *
 * `V` is a vertex payload type and is value-initialized for the constructor's initial
 * vertices.  Weight edits are not topology edits: existing node views remain current
 * and see the new weight.  Public arc ids are never reused before clear(), which
 * avoids a second ABA channel even though resource handles are recyclable.
 */
template <class W = int, class V = monostate> class ngraph_topology {
  public:
    using weight_type = W;
    using vertex_value_type = V;
    using value_type = ngraph_node_record<W, V>;
    using info_type = monostate;
    using record_type = value_type;
    using domain_type = nnode_domain<value_type>;
    using node_view = nnode_view<ngraph_topology>;

  private:
    domain_type pool_;
    nvector<int> vertex_handles_;
    nvector<int> arc_handles_;
    int arc_count_ = 0;

    static int checked_vertices(int vertices) {
        npre(vertices >= 0);
        return vertices;
    }
    static int checked_capacity(int vertices, int expected_edges) {
        npre(expected_edges >= 0 && vertices <= INT_MAX - expected_edges);
        return vertices + expected_edges;
    }

    void initialize(int vertices, int expected_edges) {
        vertices = checked_vertices(vertices);
        expected_edges = checked_vertices(expected_edges);
        npre(vertex_handles_.len() == 0 && arc_handles_.len() == 0);
        vertex_handles_.reserve(vertices);
        arc_handles_.reserve(expected_edges);
        pool_.reserve(checked_capacity(vertices, expected_edges));
        for (int vertex = 0; vertex < vertices; ++vertex)
            make_vertex(vertex, V{});
        if (vertices)
            touch();
    }

    void touch() noexcept { pool_.touch(); }

    int vertex_handle(int vertex) const noexcept {
        return 0 <= vertex && vertex < vertex_handles_.len() ? vertex_handles_[vertex] : 0;
    }
    int arc_handle(int id) const noexcept {
        return 0 <= id && id < arc_handles_.len() ? arc_handles_[id] : 0;
    }
    bool active_arc(int id) const noexcept { return arc_handle(id) != 0; }

    void make_vertex(int id, V payload) {
        vertex_handles_.push(pool_.make(ngraph_node_record<W, V>(ngraph_node_kind::vertex, id,
                                                                  move(payload))));
    }

    void link_arc(int handle) {
        value_type& arc = pool_[handle];
        npre(arc.kind == ngraph_node_kind::arc);
        value_type& vertex = pool_[vertex_handles_[arc.from]];
        arc.prev = vertex.tail;
        arc.next = 0;
        if (vertex.tail)
            pool_[vertex.tail].next = handle;
        else
            vertex.head = handle;
        vertex.tail = handle;
        npre(vertex.degree < INT_MAX);
        ++vertex.degree;
    }

    void unlink_arc(int handle) {
        value_type& arc = pool_[handle];
        npre(arc.kind == ngraph_node_kind::arc);
        value_type& vertex = pool_[vertex_handles_[arc.from]];
        if (arc.prev)
            pool_[arc.prev].next = arc.next;
        else
            vertex.head = arc.next;
        if (arc.next)
            pool_[arc.next].prev = arc.prev;
        else
            vertex.tail = arc.prev;
        npre(vertex.degree > 0);
        --vertex.degree;
        arc.next = arc.prev = 0;
    }

    void link_arc_front(int handle) {
        value_type& arc = pool_[handle];
        npre(arc.kind == ngraph_node_kind::arc);
        value_type& vertex = pool_[vertex_handles_[arc.from]];
        arc.prev = 0;
        arc.next = vertex.head;
        if (vertex.head)
            pool_[vertex.head].prev = handle;
        else
            vertex.tail = handle;
        vertex.head = handle;
        npre(vertex.degree < INT_MAX);
        ++vertex.degree;
    }

    int add0(int from, int to, W weight, bool append = true) {
        npre(0 <= from && from < len() && 0 <= to && to < len());
        npre(arc_handles_.len() < INT_MAX && arc_count_ < INT_MAX);
        int id = arc_handles_.len();
        int handle = pool_.make(
            ngraph_node_record<W, V>(ngraph_node_kind::arc, id, from, to, move(weight)));
        arc_handles_.push(handle);
        if (append)
            link_arc(handle);
        else
            link_arc_front(handle);
        ++arc_count_;
        return id;
    }

    bool release_edges(bool reset_ids) {
        bool changed = arc_count_ || (reset_ids && !arc_handles_.empty());
        if (!changed)
            return false;
        for (int id = 0; id < arc_handles_.len(); ++id) {
            int handle = arc_handles_[id];
            if (!handle)
                continue;
            unlink_arc(handle);
            pool_.erase(handle);
            arc_handles_[id] = 0;
        }
        arc_count_ = 0;
        if (reset_ids)
            arc_handles_.clear();
        return true;
    }

    friend class nnode_view<ngraph_topology>;
    uint64_t nnode_epoch() const noexcept { return pool_.epoch(); }
    const void* nnode_domain_token() const noexcept { return pool_.domain_token(); }
    nnode_identity nnode_identity_of(int handle) const noexcept {
        return handle ? pool_.identity(handle)
                      : nnode_identity{pool_.domain_token(), 0, 0};
    }
    bool nnode_alive(int handle) const noexcept { return pool_.alive(handle); }
    const value_type& nnode_val(int handle) const { return pool_[handle]; }
    int nnode_count(int handle) const { return handle ? 1 : 0; }
    int nnode_len(int handle) const { return handle ? 1 : 0; }
    info_type nnode_info(int) const { return {}; }
    int nnode_left(int) const noexcept { return 0; }
    int nnode_right(int) const noexcept { return 0; }

    template <bool Constant> struct adjacency_cursor {
        using graph_type = conditional_t<Constant, const ngraph_topology, ngraph_topology>;
        using weight_reference = conditional_t<Constant, const W&, W&>;
        graph_type* graph;
        int from, handle, index = 0;

        bool ok() const noexcept { return handle != 0; }
        nedge<weight_reference> val() const {
            auto& arc = graph->pool_[handle];
            return {from, arc.to, arc.id, arc.weight};
        }
        int idx() const noexcept { return index; }
        void next() {
            npre(handle);
            handle = graph->pool_[handle].next;
            ++index;
        }
    };

    template <bool Constant> class adjacency_view {
        using graph_type = conditional_t<Constant, const ngraph_topology, ngraph_topology>;
        graph_type* graph_;
        int vertex_;

      public:
        adjacency_view(graph_type* graph, int vertex) : graph_(graph), vertex_(vertex) {}
        int len() const { return graph_->pool_[graph_->vertex_handles_[vertex_]].degree; }
        bool empty() const { return !graph_->pool_[graph_->vertex_handles_[vertex_]].head; }
        auto enumerate() const {
            int handle = graph_->pool_[graph_->vertex_handles_[vertex_]].head;
            return adjacency_cursor<Constant>{graph_, vertex_, handle};
        }
    };

    template <bool Constant> struct arcs_cursor {
        using graph_type = conditional_t<Constant, const ngraph_topology, ngraph_topology>;
        using weight_reference = conditional_t<Constant, const W&, W&>;
        graph_type* graph;
        int id = 0, index = 0;

        explicit arcs_cursor(graph_type* graph) : graph(graph) { seek(); }
        void seek() {
            while (id < graph->arc_handles_.len() && !graph->arc_handles_[id])
                ++id;
        }
        bool ok() const noexcept { return id < graph->arc_handles_.len(); }
        nedge<weight_reference> val() const {
            auto& arc = graph->pool_[graph->arc_handles_[id]];
            return {arc.from, arc.to, arc.id, arc.weight};
        }
        int idx() const noexcept { return index; }
        void next() {
            npre(ok());
            ++id;
            ++index;
            seek();
        }
    };

    template <bool Constant> class arcs_view {
        using graph_type = conditional_t<Constant, const ngraph_topology, ngraph_topology>;
        graph_type* graph_;

      public:
        explicit arcs_view(graph_type* graph) : graph_(graph) {}
        int len() const noexcept { return graph_->arc_count_; }
        bool empty() const noexcept { return !graph_->arc_count_; }
        auto enumerate() const { return arcs_cursor<Constant>(graph_); }
    };

  public:
    using edge = nedge<W>;
    using view = adjacency_view<false>;
    using const_view = adjacency_view<true>;

    ngraph_topology() = default;
    explicit ngraph_topology(int vertices, int expected_edges = 0) {
        initialize(vertices, expected_edges);
    }
    explicit ngraph_topology(domain_type domain, int vertices = 0, int expected_edges = 0)
        : pool_(move(domain)) {
        initialize(vertices, expected_edges);
    }

    ngraph_topology(const ngraph_topology& other)
        : pool_(other.pool_.clone()), vertex_handles_(other.vertex_handles_),
          arc_handles_(other.arc_handles_), arc_count_(other.arc_count_) {}
    ngraph_topology(ngraph_topology&& other) noexcept(
        is_nothrow_move_constructible_v<domain_type> && is_nothrow_move_constructible_v<nvector<int>>)
        : pool_(move(other.pool_)), vertex_handles_(move(other.vertex_handles_)),
          arc_handles_(move(other.arc_handles_)), arc_count_(exchange(other.arc_count_, 0)) {
        other.touch();
    }
    ngraph_topology& operator=(const ngraph_topology& other) {
        if (this != addressof(other)) {
            pool_ = other.pool_.clone();
            vertex_handles_ = other.vertex_handles_;
            arc_handles_ = other.arc_handles_;
            arc_count_ = other.arc_count_;
            touch();
        }
        return *this;
    }
    ngraph_topology& operator=(ngraph_topology&& other) noexcept(
        is_nothrow_move_assignable_v<domain_type> && is_nothrow_move_assignable_v<nvector<int>>) {
        if (this != addressof(other)) {
            pool_ = move(other.pool_);
            vertex_handles_ = move(other.vertex_handles_);
            arc_handles_ = move(other.arc_handles_);
            arc_count_ = exchange(other.arc_count_, 0);
            touch();
            other.touch();
        }
        return *this;
    }

    int len() const noexcept { return vertex_handles_.len(); }
    bool empty() const noexcept { return !len(); }
    int edges() const noexcept { return arc_count_; }
    auto vertices() const { return nrange(len()); }
    domain_type domain() const { return pool_; }
    bool same_domain(const ngraph_topology& other) const noexcept {
        return pool_.same_domain(other.pool_);
    }
    void reserve(int expected_edges) {
        expected_edges = checked_vertices(expected_edges);
        arc_handles_.reserve(expected_edges);
        pool_.reserve(checked_capacity(len(), expected_edges));
    }

    int add_vertex(V payload = V{}) {
        npre(vertex_handles_.len() < INT_MAX);
        int id = vertex_handles_.len();
        make_vertex(id, move(payload));
        touch();
        return id;
    }
    V& vertex_value(int vertex) {
        npre(0 <= vertex && vertex < len());
        value_type& record = pool_[vertex_handles_[vertex]];
        npre(record.kind == ngraph_node_kind::vertex && record.payload.has_value());
        return *record.payload;
    }
    const V& vertex_value(int vertex) const {
        npre(0 <= vertex && vertex < len());
        const value_type& record = pool_[vertex_handles_[vertex]];
        npre(record.kind == ngraph_node_kind::vertex && record.payload.has_value());
        return *record.payload;
    }
    void set_vertex(int vertex, V payload) { vertex_value(vertex) = move(payload); }

    int add(int from, int to, W weight = W{1}) {
        int id = add0(from, to, move(weight));
        touch();
        return id;
    }
    // Compatibility backends may require head insertion.  Public graph algorithms should
    // use add(), whose order is append order; this hook preserves an older owner's exact
    // adjacency order without duplicating the resource record or inventing a graph view.
    int add_front(int from, int to, W weight = W{1}) {
        int id = add0(from, to, move(weight), false);
        touch();
        return id;
    }
    pair<int, int> add2(int a, int b, const W& weight = W{1}) {
        int forward = add0(a, b, W(weight));
        int backward = add0(b, a, W(weight));
        touch();
        return {forward, backward};
    }
    bool erase(int id) {
        int handle = arc_handle(id);
        if (!handle)
            return false;
        unlink_arc(handle);
        pool_.erase(handle);
        arc_handles_[id] = 0;
        --arc_count_;
        touch();
        return true;
    }
    bool del(int id) { return erase(id); }
    bool rewire(int id, int from, int to) {
        int handle = arc_handle(id);
        if (!handle)
            return false;
        npre(0 <= from && from < len() && 0 <= to && to < len());
        value_type& arc = pool_[handle];
        if (arc.from == from && arc.to == to)
            return true;
        unlink_arc(handle);
        arc.from = from;
        arc.to = to;
        link_arc(handle);
        touch();
        return true;
    }
    void clear_edges() {
        if (release_edges(false))
            touch();
    }
    void reset_edges() {
        if (release_edges(true))
            touch();
    }
    void clear() {
        bool changed = len() || arc_count_ || !arc_handles_.empty();
        if (!changed)
            return;
        release_edges(true);
        for (int index = 0; index < vertex_handles_.len(); ++index)
            pool_.erase(vertex_handles_[index]);
        vertex_handles_.clear();
        arc_handles_.clear();
        arc_count_ = 0;
        touch();
    }

    node_view vertex_node(int vertex) const {
        npre(0 <= vertex && vertex < len());
        return node_view(this, vertex_handles_[vertex], pool_.epoch());
    }
    node_view arc_node(int id) const {
        int handle = arc_handle(id);
        npre(handle);
        return node_view(this, handle, pool_.epoch());
    }

    adjacency_view<false> neighbors(int vertex) {
        npre(0 <= vertex && vertex < len());
        return {this, vertex};
    }
    adjacency_view<true> neighbors(int vertex) const {
        npre(0 <= vertex && vertex < len());
        return {this, vertex};
    }
    adjacency_view<false> operator[](int vertex) & { return neighbors(vertex); }
    adjacency_view<true> operator[](int vertex) const& { return neighbors(vertex); }
    adjacency_view<false> operator[](int) && = delete;
    adjacency_view<true> operator[](int) const&& = delete;
    adjacency_view<false> from(int vertex) & { return neighbors(vertex); }
    adjacency_view<true> from(int vertex) const& { return neighbors(vertex); }
    adjacency_view<false> from(int) && = delete;
    adjacency_view<true> from(int) const&& = delete;

    int degree(int vertex) const { return neighbors(vertex).len(); }
    int find(int from, int to, int fallback = npos) const {
        npre(0 <= from && from < len() && 0 <= to && to < len());
        nfor(edge, neighbors(from))
            if (edge.to == to)
                return edge.id;
        return fallback;
    }
    bool has(int from, int to) const { return find(from, to) != npos; }
    W* weight(int id) {
        int handle = arc_handle(id);
        return handle ? addressof(pool_[handle].weight) : nullptr;
    }
    const W* weight(int id) const {
        int handle = arc_handle(id);
        return handle ? addressof(pool_[handle].weight) : nullptr;
    }
    W weight(int id, W fallback) const {
        const W* value = weight(id);
        return value ? *value : move(fallback);
    }
    bool set(int id, W weight) {
        int handle = arc_handle(id);
        if (!handle)
            return false;
        pool_[handle].weight = move(weight);
        return true;
    }

    auto arcs() & { return arcs_view<false>(this); }
    auto arcs() const& { return arcs_view<true>(this); }
    auto arcs() && = delete;
    auto arcs() const&& = delete;
};

// Compatibility forward-star facade over the resource-backed topology.  The old backend
// prepended each new arc to its source list; add_front() keeps that observable order while
// the node records, weights and adjacency links now come from the shared resource layer.
// `clear_edges()` deliberately resets the old dense edge-id namespace, unlike topology's
// stable-id `clear_edges()`; this is the historical ngraph_forward contract.
template <class W = int> class ngraph_forward {
    using topology_type = ngraph_topology<W>;
    topology_type topology_;

    template <bool Constant> struct arcs_cursor {
        using graph_type = conditional_t<Constant, const ngraph_forward, ngraph_forward>;
        using adjacency_type = conditional_t<Constant, typename topology_type::const_view,
                                             typename topology_type::view>;
        using inner_cursor = nenumerator_t<adjacency_type>;

        graph_type* graph;
        int vertex = 0, index = 0;
        optional<inner_cursor> inner;

        explicit arcs_cursor(graph_type* graph) : graph(graph) { seek(); }
        void seek() {
            while (vertex < graph->len()) {
                inner.emplace(nenumerate(graph->topology_.neighbors(vertex)));
                if (inner->ok())
                    return;
                ++vertex;
            }
            inner.reset();
        }
        bool ok() const noexcept { return inner.has_value(); }
        auto val() {
            decltype(auto) raw = inner->val();
            using weight_reference = decltype(nedge_weight(raw));
            return nedge<weight_reference>{vertex, nedge_to(raw), raw.id, nedge_weight(raw)};
        }
        int idx() const noexcept { return index; }
        void next() {
            npre(ok());
            inner->next();
            ++index;
            if (!inner->ok()) {
                ++vertex;
                seek();
            }
        }
    };

    template <bool Constant> class arcs_view {
        using graph_type = conditional_t<Constant, const ngraph_forward, ngraph_forward>;
        graph_type* graph_;

      public:
        explicit arcs_view(graph_type* graph) : graph_(graph) {}
        int len() const noexcept { return graph_->edges(); }
        bool empty() const noexcept { return !graph_->edges(); }
        auto enumerate() const { return arcs_cursor<Constant>(graph_); }
    };

  public:
    using edge = nedge<W>;
    using view = typename topology_type::view;
    using const_view = typename topology_type::const_view;

    ngraph_forward() = default;
    explicit ngraph_forward(int vertices, int expected_edges = 0)
        : topology_(vertices, expected_edges) {}

    int len() const noexcept { return topology_.len(); }
    int edges() const noexcept { return topology_.edges(); }
    bool empty() const noexcept { return topology_.empty(); }
    void reserve(int expected_edges) { topology_.reserve(expected_edges); }
    void clear_edges() { topology_.reset_edges(); }

    int add(int from, int to, W weight = W{1}) {
        return topology_.add_front(from, to, move(weight));
    }
    pair<int, int> add2(int a, int b, W weight = W{1}) {
        int forward = add(a, b, weight);
        int backward = add(b, a, move(weight));
        return {forward, backward};
    }

    view neighbors(int vertex) { return topology_.neighbors(vertex); }
    const_view neighbors(int vertex) const { return topology_.neighbors(vertex); }
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
        return topology_.find(from, to, fallback);
    }
    bool has(int from, int to) const { return topology_.has(from, to); }
    W* weight(int edge) { return topology_.weight(edge); }
    const W* weight(int edge) const { return topology_.weight(edge); }
    W weight(int edge, W fallback) const { return topology_.weight(edge, move(fallback)); }
    bool set(int edge, W weight) { return topology_.set(edge, move(weight)); }

    auto vertices() const { return topology_.vertices(); }
    auto arcs() & { return arcs_view<false>(this); }
    auto arcs() const& { return arcs_view<true>(this); }
    auto arcs() && = delete;
    auto arcs() const&& = delete;

    ngraph_forward reverse() const {
        ngraph_forward result(len(), edges());
        nfor(edge, arcs())
            result.add(edge.to, edge.from, edge.w);
        return result;
    }
};

/**
 * Immutable CSR graph built from a graph-like source.  Source destinations must be in
 * range; the CSR owner copies topology and iteration order is preserved from input.
 */
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

// Flattened borrowed arc view.  The graph owner must outlive enumeration and must not
// mutate adjacency while a cursor is active; arc order is backend iteration order.
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

// Borrowed filtered graph view.  P must be stable and side-effect-safe across repeated
// neighbor enumeration; the source graph must outlive the view and stay structurally valid.
template <class G, class P> class ngraph_where_view {
    const G* graph_;
    [[no_unique_address]] P predicate_;
    using adjacency_result = decltype(declval<const G&>().neighbors(0));
    using base_cursor = nenumerator_t<adjacency_result>;

    struct filtered_cursor {
        base_cursor base;
        const P* predicate;
        int index = 0;

        filtered_cursor(base_cursor base, const P* predicate)
            : base(move(base)), predicate(predicate) {
            skip();
        }
        void skip() {
            while (base.ok() && !invoke(*predicate, base.val()))
                base.next();
        }
        bool ok() const { return base.ok(); }
        decltype(auto) val() { return base.val(); }
        int idx() const { return index; }
        void next() {
            base.next();
            ++index;
            skip();
        }
    };

    class adjacency_view {
        const ngraph_where_view* owner_;
        int vertex_;

      public:
        adjacency_view(const ngraph_where_view* owner, int vertex)
            : owner_(owner), vertex_(vertex) {}
        auto enumerate() const {
            return filtered_cursor(nenumerate(owner_->graph_->neighbors(vertex_)),
                                   addressof(owner_->predicate_));
        }
        int len() const {
            int result = 0;
            auto cursor = enumerate();
            while (cursor.ok()) {
                npre(result < INT_MAX);
                ++result;
                cursor.next();
            }
            return result;
        }
        bool empty() const {
            auto cursor = enumerate();
            return !cursor.ok();
        }
    };

  public:
    ngraph_where_view(const G& graph, P predicate)
        : graph_(addressof(graph)), predicate_(move(predicate)) {}
    int len() const { return ni::ngraph_vertices(*graph_); }
    bool empty() const { return len() == 0; }
    auto vertices() const { return nrange(len()); }
    adjacency_view neighbors(int vertex) const {
        npre(0 <= vertex && vertex < len());
        return {this, vertex};
    }
    adjacency_view operator[](int vertex) const& { return neighbors(vertex); }
    adjacency_view operator[](int) const&& = delete;
    adjacency_view from(int vertex) const& { return neighbors(vertex); }
    adjacency_view from(int) const&& = delete;
    int edges() const {
        int result = 0;
        for (int vertex = 0; vertex < len(); ++vertex) {
            int degree = neighbors(vertex).len();
            npre(result <= INT_MAX - degree);
            result += degree;
        }
        return result;
    }
    auto arcs() const& { return narcs(*this); }
    auto arcs() const&& = delete;
};

template <ngraph_like G, class P> auto ngraph_where(const G& graph, P predicate) {
    return ngraph_where_view<G, P>(graph, move(predicate));
}
template <ngraph_like G, class P> auto ngraph_where(const G&&, P) = delete;

// Path result uses parent pointers over one fixed vertex universe.  `reachable` is the
// only authority for path validity; a missing parent is valid at the source.
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

// Dijkstra requires every traversed edge weight to be nonnegative and all additions to
// stay below the chosen infinity sentinel.  Negative edges invalidate the greedy proof.
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
