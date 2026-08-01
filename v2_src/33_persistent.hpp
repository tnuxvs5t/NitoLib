template <class T, class O = nadd<T>>
    requires nmonoid<O, T> && copyable<T>
class npersistent_seg {
    struct node {
        T aggregate;
        int left = 0, right = 0;
    };

    [[no_unique_address]] O operation_;
    int size_ = 0, base_ = 1;
    vector<node> nodes_;
    vector<int> roots_;

    static int checked_size(int n) {
        npre(0 <= n && n <= (1 << 30));
        return n;
    }

    const T& aggregate(int node_index) const { return nodes_[node_index].aggregate; }

    int append(node value) {
        npre(nodes_.size() <= size_t(INT_MAX));
        int index = int(nodes_.size());
        nodes_.push_back(move(value));
        return index;
    }

    template <class V> int build(const V& source, int left, int right) {
        if (left >= size_)
            return 0;
        if (right - left == 1)
            return append({source[left], 0, 0});
        int middle = left + (right - left) / 2;
        int left_node = build(source, left, middle);
        int right_node = build(source, middle, right);
        return append({operation_(aggregate(left_node), aggregate(right_node)), left_node, right_node});
    }

    int set0(int current, int left, int right, int index, const T& value) {
        if (right - left == 1)
            return append({value, 0, 0});
        int left_node = nodes_[current].left;
        int right_node = nodes_[current].right;
        int middle = left + (right - left) / 2;
        if (index < middle)
            left_node = set0(left_node, left, middle, index, value);
        else
            right_node = set0(right_node, middle, right, index, value);
        return append({operation_(aggregate(left_node), aggregate(right_node)), left_node, right_node});
    }

    T fold0(int current, int left, int right, int query_left, int query_right) const {
        if (!current || query_right <= left || right <= query_left)
            return operation_.id();
        if (query_left <= left && right <= query_right)
            return aggregate(current);
        int middle = left + (right - left) / 2;
        return operation_(fold0(nodes_[current].left, left, middle, query_left, query_right),
                          fold0(nodes_[current].right, middle, right, query_left, query_right));
    }

  public:
    explicit npersistent_seg(int n = 0, O operation = {})
        : operation_(move(operation)), size_(checked_size(n)), base_(nbitceil(size_)),
          nodes_{{operation_.id(), 0, 0}}, roots_{0} {}

    template <nindexed V>
    explicit npersistent_seg(const V& source, O operation = {})
        : operation_(move(operation)), size_(checked_size(nlen(source))), base_(nbitceil(size_)),
          nodes_{{operation_.id(), 0, 0}} {
        roots_.push_back(size_ ? build(source, 0, base_) : 0);
    }

    int len() const noexcept { return size_; }
    int versions() const noexcept {
        npre(roots_.size() <= size_t(INT_MAX));
        return int(roots_.size());
    }
    int nodes() const noexcept {
        npre(0 < nodes_.size() && nodes_.size() <= size_t(INT_MAX) + 1);
        return int(nodes_.size() - 1);
    }
    const O& operation() const noexcept { return operation_; }

    void reserve_nodes(int count) {
        npre(count >= 0);
        nodes_.reserve(size_t(count) + 1);
    }

    int fork(int version) {
        npre(0 <= version && version < versions());
        npre(roots_.size() < size_t(INT_MAX));
        roots_.push_back(roots_[version]);
        return versions() - 1;
    }

    int set(int version, int index, const T& value) {
        npre(0 <= version && version < versions());
        npre(0 <= index && index < size_);
        npre(roots_.size() < size_t(INT_MAX));
        int root = set0(roots_[version], 0, base_, index, value);
        roots_.push_back(root);
        return versions() - 1;
    }

    T fold(int version, int left, int right) const {
        npre(0 <= version && version < versions());
        npre(0 <= left && left <= right && right <= size_);
        return fold0(roots_[version], 0, base_, left, right);
    }
    T fold(int version) const { return fold(version, 0, size_); }
    T get(int version, int index) const {
        npre(0 <= index && index < size_);
        return fold(version, index, index + 1);
    }
};
