template <ngraph_like G> nvector<int> n01bfs(const G& graph, int source) {
    int vertices = graph.vertices();
    npre(0 <= source && source < vertices);
    nvector<int> distance(vertices, npos);
    ndeque<int> queue;
    distance[source] = 0;
    queue.pushr(source);
    while (!queue.empty()) {
        int from = queue.popl();
        auto adjacency = graph.neighbors(from);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            int weight = int(nedge_weight(edge));
            npre(0 <= to && to < vertices && (weight == 0 || weight == 1));
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
    requires is_arithmetic_v<D>
nmaybe<nmst_result<D>> nprim(const G& graph, int root = 0) {
    int vertices = graph.vertices();
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
            result.weight += weight;
            result.edges.push(pair<int, int>{parent, vertex});
        }
        auto adjacency = graph.neighbors(vertex);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            npre(0 <= to && to < vertices);
            if (!used[to])
                queue.push({D(nedge_weight(edge)), to, vertex});
        }
    }
    return visited == vertices ? nmaybe<nmst_result<D>>(move(result)) : nmaybe<nmst_result<D>>{};
}

template <class C>
    requires is_arithmetic_v<C>
class nmaxflow {
    struct edge {
        int to, reverse;
        C capacity;
    };
    vector<vector<edge>> graph_;
    bool flowed_ = false;

    static size_t checked_vertices(int vertices) {
        npre(vertices >= 0);
        return size_t(vertices);
    }

  public:
    explicit nmaxflow(int vertices = 0) : graph_(checked_vertices(vertices)) {}
    int vertices() const noexcept { return int(graph_.size()); }

    void add(int from, int to, C capacity) {
        npre(!flowed_);
        npre(0 <= from && from < vertices() && 0 <= to && to < vertices() && from != to);
        npre(!(capacity < C{}));
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
