// Matching arrays use dense left/right universes and npos for unmatched vertices.
// Every matched pair must be mutual across the two arrays.
struct nbipartite_matching {
    int size = 0;
    nvector<int> left, right;
};

// Graph vertices form the left part and every edge destination belongs to the separate
// [0,right_vertices) part; edges within one side violate the bipartite model.
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

struct nbicover {
    nvector<int> l, r;
};

// Stateful Hopcroft-Karp owner.  add() uses distinct dense left/right id spaces;
// solve() invalidates earlier matching/cover snapshots after graph mutation.
class nbimatch_hopcroft {
    int left_vertices_ = 0, right_vertices_ = 0;
    nvector<nvector<int>> adjacency_;
    nvector<int> left_match_, right_match_;
    bool solved_ = false;

  public:
    nbimatch_hopcroft() = default;
    nbimatch_hopcroft(int left_vertices, int right_vertices)
        : left_vertices_(left_vertices), right_vertices_(right_vertices),
          adjacency_(left_vertices), left_match_(left_vertices, npos),
          right_match_(right_vertices, npos) {
        npre(left_vertices >= 0 && right_vertices >= 0);
    }

    int add(int left, int right) {
        npre(0 <= left && left < left_vertices_ && 0 <= right && right < right_vertices_);
        solved_ = false;
        adjacency_[left].push(right);
        return adjacency_[left].len() - 1;
    }
    int solve() {
        auto graph = ngraph_view(left_vertices_,
                                 [this](int left) -> const nvector<int>& { return adjacency_[left]; });
        auto result = nhopcroft_karp(graph, right_vertices_);
        left_match_ = move(result.left);
        right_match_ = move(result.right);
        solved_ = true;
        return result.size;
    }
    int left(int vertex, int fallback = npos) const {
        npre(0 <= vertex && vertex < left_vertices_);
        return left_match_[vertex] == npos ? fallback : left_match_[vertex];
    }
    int right(int vertex, int fallback = npos) const {
        npre(0 <= vertex && vertex < right_vertices_);
        return right_match_[vertex] == npos ? fallback : right_match_[vertex];
    }
    nvector<pair<int, int>> pairs() const {
        npre(solved_);
        nvector<pair<int, int>> result;
        for (int left = 0; left < left_vertices_; ++left)
            if (left_match_[left] != npos)
                result.push(pair<int, int>{left, left_match_[left]});
        return result;
    }
    nbicover mincover() const {
        npre(solved_);
        nvector<unsigned char> reachable_left(left_vertices_, false),
            reachable_right(right_vertices_, false);
        deque<int> queue;
        for (int left = 0; left < left_vertices_; ++left)
            if (left_match_[left] == npos) {
                reachable_left[left] = true;
                queue.push_back(left);
            }
        while (!queue.empty()) {
            int left = queue.front();
            queue.pop_front();
            for (int index = 0; index < adjacency_[left].len(); ++index) {
                int right = adjacency_[left][index];
                if (left_match_[left] == right || reachable_right[right])
                    continue;
                reachable_right[right] = true;
                int next_left = right_match_[right];
                if (next_left != npos && !reachable_left[next_left]) {
                    reachable_left[next_left] = true;
                    queue.push_back(next_left);
                }
            }
        }
        nbicover result;
        for (int left = 0; left < left_vertices_; ++left)
            if (!reachable_left[left])
                result.l.push(left);
        for (int right = 0; right < right_vertices_; ++right)
            if (reachable_right[right])
                result.r.push(right);
        return result;
    }
};

using nbimatch = nbimatch_hopcroft;
