template <class T, class O = nadd<T>>
    requires nmonoid<O, T> && copyable<T>
class nqueue_agg {
    struct node {
        T value;
        T aggregate;
    };
    [[no_unique_address]] O operation_;
    vector<node> left_, right_;

    void transfer() {
        if (!left_.empty())
            return;
        while (!right_.empty()) {
            T value = move(right_.back().value);
            right_.pop_back();
            T aggregate = left_.empty() ? value : operation_(value, left_.back().aggregate);
            left_.push_back({move(value), move(aggregate)});
        }
    }

  public:
    nqueue_agg() = default;
    explicit nqueue_agg(O operation) : operation_(move(operation)) {}

    int len() const noexcept {
        npre(left_.size() <= size_t(INT_MAX));
        npre(right_.size() <= size_t(INT_MAX) - left_.size());
        return int(left_.size() + right_.size());
    }
    bool empty() const noexcept { return left_.empty() && right_.empty(); }

    void push(T value) {
        npre(len() < INT_MAX);
        T aggregate = right_.empty() ? value : operation_(right_.back().aggregate, value);
        right_.push_back({move(value), move(aggregate)});
    }

    const T& front() {
        npre(!empty());
        return left_.empty() ? right_.front().value : left_.back().value;
    }
    const T& front() const {
        npre(!empty());
        return left_.empty() ? right_.front().value : left_.back().value;
    }

    T pop() {
        npre(!empty());
        transfer();
        T value = move(left_.back().value);
        left_.pop_back();
        return value;
    }
    T pop(T fallback) { return empty() ? move(fallback) : pop(); }

    T fold() const {
        if (left_.empty())
            return right_.empty() ? operation_.id() : right_.back().aggregate;
        if (right_.empty())
            return left_.back().aggregate;
        return operation_(left_.back().aggregate, right_.back().aggregate);
    }
};

class ndsu {
    vector<int> parent_;

    static size_t checked_size(int n) {
        npre(n >= 0);
        return size_t(n);
    }

  public:
    explicit ndsu(int n = 0) : parent_(checked_size(n), -1) {}
    int len() const noexcept { return int(parent_.size()); }

    int find(int vertex) {
        npre(0 <= vertex && vertex < len());
        int root = vertex;
        while (parent_[root] >= 0)
            root = parent_[root];
        while (vertex != root) {
            int next = parent_[vertex];
            parent_[vertex] = root;
            vertex = next;
        }
        return root;
    }
    int operator()(int vertex) { return find(vertex); }
    int size(int vertex) { return -parent_[find(vertex)]; }
    bool same(int a, int b) { return find(a) == find(b); }

    int merge(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b)
            return a;
        if (parent_[a] > parent_[b])
            swap(a, b);
        parent_[a] += parent_[b];
        parent_[b] = a;
        return a;
    }

    nvector<int> partition() {
        nvector<int> root_class(len(), npos), classes(len());
        int count = 0;
        for (int vertex = 0; vertex < len(); ++vertex) {
            int root = find(vertex);
            if (root_class[root] == npos)
                root_class[root] = count++;
            classes[vertex] = root_class[root];
        }
        return classes;
    }
};

class nrollback_dsu {
    struct change {
        int large, large_parent, small, small_parent;
    };
    vector<int> parent_;
    vector<change> history_;

    static size_t checked_size(int n) {
        npre(n >= 0);
        return size_t(n);
    }

  public:
    explicit nrollback_dsu(int n = 0) : parent_(checked_size(n), -1) {}
    int len() const noexcept { return int(parent_.size()); }
    int time() const noexcept { return int(history_.size()); }

    int find(int vertex) const {
        npre(0 <= vertex && vertex < len());
        while (parent_[vertex] >= 0)
            vertex = parent_[vertex];
        return vertex;
    }
    int size(int vertex) const { return -parent_[find(vertex)]; }
    bool same(int a, int b) const { return find(a) == find(b); }

    bool merge(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b)
            return false;
        if (parent_[a] > parent_[b])
            swap(a, b);
        history_.push_back({a, parent_[a], b, parent_[b]});
        parent_[a] += parent_[b];
        parent_[b] = a;
        return true;
    }

    void undo() {
        npre(!history_.empty());
        auto change = history_.back();
        history_.pop_back();
        parent_[change.large] = change.large_parent;
        parent_[change.small] = change.small_parent;
    }
    void rollback(int checkpoint) {
        npre(0 <= checkpoint && checkpoint <= time());
        while (time() > checkpoint)
            undo();
    }
};

using ndsu_rollback = nrollback_dsu;
