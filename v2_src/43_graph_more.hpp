template <ngraph_like G> nvector<int> n01bfs(const G& graph, int source) {
    int vertices = ni::ngraph_vertices(graph);
    npre(0 <= source && source < vertices);
    nvector<int> distance(vertices, npos);
    ndeque<int> queue;
    distance[source] = 0;
    queue.pushr(source);
    while (!queue.empty()) {
        int from = queue.popl();
        decltype(auto) adjacency = graph.neighbors(from);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            auto&& raw_weight = nedge_weight(edge);
            npre(raw_weight == 0 || raw_weight == 1);
            int weight = int(raw_weight);
            npre(0 <= to && to < vertices);
            int candidate = distance[from] + weight;
            if (distance[to] == npos || candidate < distance[to]) {
                distance[to] = candidate;
                weight ? queue.pushr(to) : queue.pushl(to);
            }
        }
    }
    return distance;
}

template <class W> struct nmst_result {
    W weight{};
    nvector<pair<int, int>> edges;
};

template <class D = long long, ngraph_like G>
    requires is_arithmetic_v<D> && (!same_as<remove_cv_t<D>, bool>)
nmaybe<nmst_result<D>> nprim(const G& graph, int root = 0) {
    int vertices = ni::ngraph_vertices(graph);
    if (!vertices)
        return nmst_result<D>{};
    npre(0 <= root && root < vertices);
    using candidate = tuple<D, int, int>;
    priority_queue<candidate, vector<candidate>, greater<candidate>> queue;
    nvector<unsigned char> used(vertices, false);
    nmst_result<D> result;
    result.edges.reserve(vertices - 1);
    queue.push({D{}, root, npos});
    int visited = 0;
    while (!queue.empty()) {
        auto [weight, vertex, parent] = queue.top();
        queue.pop();
        if (used[vertex])
            continue;
        used[vertex] = true;
        ++visited;
        if (parent != npos) {
            result.weight = ni::nchecked_add(result.weight, weight);
            result.edges.push(pair<int, int>{parent, vertex});
        }
        decltype(auto) adjacency = graph.neighbors(vertex);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            npre(0 <= to && to < vertices);
            if (!used[to])
                queue.push({ni::nchecked_number<D>(nedge_weight(edge)), to, vertex});
        }
    }
    return visited == vertices ? nmaybe<nmst_result<D>>(move(result)) : nmaybe<nmst_result<D>>{};
}

template <class C>
    requires integral<C> && (!same_as<remove_cv_t<C>, bool>)
class nmaxflow {
    struct edge {
        int to, reverse;
        C capacity;
    };
    vector<vector<edge>> graph_;
    bool flowed_ = false;

    static size_t checked_vertices(int vertices) {
        npre(0 <= vertices && vertices <= INT_MAX / 2);
        return size_t(vertices);
    }

  public:
    explicit nmaxflow(int vertices = 0) : graph_(checked_vertices(vertices)) {}
    int vertices() const noexcept { return int(graph_.size()); }

    void add(int from, int to, C capacity) {
        npre(!flowed_);
        npre(0 <= from && from < vertices() && 0 <= to && to < vertices() && from != to);
        npre(!(capacity < C{}));
        npre(graph_[to].size() < size_t(INT_MAX) && graph_[from].size() < size_t(INT_MAX));
        int from_reverse = int(graph_[to].size());
        int to_reverse = int(graph_[from].size());
        graph_[from].push_back({to, from_reverse, capacity});
        graph_[to].push_back({from, to_reverse, C{}});
    }

    C flow(int source, int sink) {
        npre(!flowed_);
        npre(0 <= source && source < vertices() && 0 <= sink && sink < vertices() && source != sink);
        flowed_ = true;
        int n = vertices();
        vector<int> height(size_t(n), 0), current(size_t(n), 0);
        vector<C> excess(size_t(n), C{});
        vector<unsigned char> queued(size_t(n), false);
        deque<int> active;

        auto activate = [&](int vertex) {
            if (vertex != source && vertex != sink && !queued[vertex] && C{} < excess[vertex]) {
                queued[vertex] = true;
                active.push_back(vertex);
            }
        };
        auto push = [&](int from, edge& arc) {
            C sent = min(excess[from], arc.capacity);
            if (!(C{} < sent) || height[from] != height[arc.to] + 1)
                return false;
            npre(graph_[arc.to][arc.reverse].capacity <= numeric_limits<C>::max() - sent);
            npre(excess[arc.to] <= numeric_limits<C>::max() - sent);
            arc.capacity -= sent;
            graph_[arc.to][arc.reverse].capacity += sent;
            excess[from] -= sent;
            excess[arc.to] += sent;
            activate(arc.to);
            return true;
        };

        height[source] = n;
        for (edge& arc : graph_[source]) {
            C sent = arc.capacity;
            if (!(C{} < sent))
                continue;
            npre(graph_[arc.to][arc.reverse].capacity <= numeric_limits<C>::max() - sent);
            npre(excess[arc.to] <= numeric_limits<C>::max() - sent);
            arc.capacity = C{};
            graph_[arc.to][arc.reverse].capacity += sent;
            excess[arc.to] += sent;
            activate(arc.to);
        }

        while (!active.empty()) {
            int vertex = active.front();
            active.pop_front();
            queued[vertex] = false;
            while (C{} < excess[vertex]) {
                if (current[vertex] == int(graph_[vertex].size())) {
                    int next_height = 2 * n;
                    for (const edge& arc : graph_[vertex])
                        if (C{} < arc.capacity)
                            nchmin(next_height, height[arc.to] + 1);
                    npre(next_height < 2 * n);
                    height[vertex] = next_height;
                    current[vertex] = 0;
                    continue;
                }
                edge& arc = graph_[vertex][current[vertex]];
                if (!push(vertex, arc))
                    ++current[vertex];
            }
        }
        return excess[sink];
    }

    nvector<unsigned char> mincut(int source) const {
        npre(flowed_ && 0 <= source && source < vertices());
        nvector<unsigned char> reachable(vertices(), false);
        ndeque<int> queue;
        reachable[source] = true;
        queue.pushr(source);
        while (!queue.empty()) {
            int from = queue.popl();
            for (const edge& arc : graph_[from])
                if (C{} < arc.capacity && !reachable[arc.to]) {
                    reachable[arc.to] = true;
                    queue.pushr(arc.to);
                }
        }
        return reachable;
    }
};

template <class C>
    requires integral<C> && (!same_as<remove_cv_t<C>, bool>)
class nflow_dinic {
    int vertices_ = 0;
    vector<int> head_, to_, next_, level_, current_;
    vector<C> residual_, initial_;

    static size_t checked_vertices(int vertices) {
        npre(vertices >= 0);
        return size_t(vertices);
    }

    bool build_levels(int source, int sink) {
        fill(level_.begin(), level_.end(), npos);
        deque<int> queue;
        level_[source] = 0;
        queue.push_back(source);
        while (!queue.empty()) {
            int from = queue.front();
            queue.pop_front();
            for (int edge = head_[from]; edge != npos; edge = next_[edge])
                if (C{} < residual_[edge] && level_[to_[edge]] == npos) {
                    level_[to_[edge]] = level_[from] + 1;
                    queue.push_back(to_[edge]);
                }
        }
        return level_[sink] != npos;
    }

    C send(int from, int sink, C available) {
        if (from == sink)
            return available;
        for (int& edge = current_[from]; edge != npos; edge = next_[edge]) {
            int to = to_[edge];
            if (!(C{} < residual_[edge]) || level_[to] != level_[from] + 1)
                continue;
            C pushed = send(to, sink, min(available, residual_[edge]));
            if (C{} < pushed) {
                npre(residual_[edge ^ 1] <= numeric_limits<C>::max() - pushed);
                residual_[edge] -= pushed;
                residual_[edge ^ 1] += pushed;
                return pushed;
            }
        }
        return C{};
    }

  public:
    nflow_dinic() = default;
    explicit nflow_dinic(int vertices, int expected_edges = 0)
        : vertices_(vertices), head_(checked_vertices(vertices), npos),
          level_(size_t(vertices)), current_(size_t(vertices)) {
        reserve(expected_edges);
    }

    int len() const noexcept { return vertices_; }
    int edges() const {
        npre(to_.size() / 2 <= size_t(INT_MAX));
        return int(to_.size() / 2);
    }
    void reserve(int expected_edges) {
        npre(expected_edges >= 0 && expected_edges <= INT_MAX / 2);
        size_t arcs = size_t(expected_edges) * 2;
        to_.reserve(arcs);
        next_.reserve(arcs);
        residual_.reserve(arcs);
        initial_.reserve(arcs);
    }
    int add(int from, int to, C capacity, C reverse_capacity = C{}) {
        npre(0 <= from && from < vertices_ && 0 <= to && to < vertices_);
        npre(!(capacity < C{}) && !(reverse_capacity < C{}));
        npre(to_.size() <= size_t(INT_MAX - 2));
        int id = edges();
        to_.push_back(to);
        next_.push_back(head_[from]);
        residual_.push_back(capacity);
        initial_.push_back(capacity);
        head_[from] = int(to_.size()) - 1;
        to_.push_back(from);
        next_.push_back(head_[to]);
        residual_.push_back(reverse_capacity);
        initial_.push_back(reverse_capacity);
        head_[to] = int(to_.size()) - 1;
        return id;
    }
    C flow(int source, int sink, C limit = numeric_limits<C>::max()) {
        npre(0 <= source && source < vertices_ && 0 <= sink && sink < vertices_ && source != sink);
        npre(!(limit < C{}));
        C result{};
        while (result < limit && build_levels(source, sink)) {
            current_ = head_;
            while (result < limit) {
                C pushed = send(source, sink, limit - result);
                if (!(C{} < pushed))
                    break;
                result += pushed;
            }
        }
        return result;
    }
    C operator()(int source, int sink, C limit = numeric_limits<C>::max()) {
        return flow(source, sink, limit);
    }
    C used(int id) const {
        npre(0 <= id && id < edges());
        return initial_[id * 2] - residual_[id * 2];
    }
    nvector<unsigned char> cut(int source) const {
        npre(0 <= source && source < vertices_);
        nvector<unsigned char> reachable(vertices_, false);
        deque<int> queue;
        reachable[source] = true;
        queue.push_back(source);
        while (!queue.empty()) {
            int from = queue.front();
            queue.pop_front();
            for (int edge = head_[from]; edge != npos; edge = next_[edge])
                if (C{} < residual_[edge] && !reachable[to_[edge]]) {
                    reachable[to_[edge]] = true;
                    queue.push_back(to_[edge]);
                }
        }
        return reachable;
    }
    void reset() { residual_ = initial_; }
};

template <class C> using nflow = nflow_dinic<C>;
