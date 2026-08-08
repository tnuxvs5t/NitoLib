/**
 * Path-copying persistent ordered segment tree.
 * O provides a two-sided identity and associative op(left,right); T and O must be
 * copyable for version sharing.  Each set appends O(log n) immutable nodes, so old
 * version views remain current until the whole owner is assigned/moved/destroyed.
 * The node domain is append-only: unlike a mutable tree, adding a version does not
 * touch its epoch and therefore cannot invalidate a view into an older version.
 */
template <class T, class O = nadd<T>> class npersistent_seg {
    struct node {
        T aggregate;
        int left = 0, right = 0;
    };

  public:
    // Persistent nodes are append-only resources.  Sharing this domain is safe
    // because an update never rewrites an existing node; copying an owner still
    // clones the domain so the old owner cannot observe a later append as its own.
    using domain_type = nnode_domain<node>;

  private:
    [[no_unique_address]] O operation_;
    int size_ = 0, base_ = 1;
    mutable domain_type pool_;
    vector<int> roots_;

    friend class nseg_node<npersistent_seg>;

    static int checked_size(int n) {
        npre(0 <= n && n <= (1 << 30));
        return n;
    }

    void touch() noexcept { pool_.touch(); }

    T aggregate(int node_index) const { return node_index ? pool_[node_index].aggregate : operation_.id(); }

    int append(node value) { return pool_.make(move(value)); }

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
        int left_node = current ? pool_[current].left : 0;
        int right_node = current ? pool_[current].right : 0;
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
        return operation_(fold0(pool_[current].left, left, middle, query_left, query_right),
                          fold0(pool_[current].right, middle, right, query_left, query_right));
    }

    // Pointwise version merge.  It is path-copying: a subtree that is absent on
    // one side is shared directly, while nodes whose two sides are present are
    // rebuilt.  `op(left_value,right_value)` is applied at every corresponding leaf.
    int merge0(int left_node, int right_node, int left, int right) {
        if (!left_node)
            return right_node;
        if (!right_node)
            return left_node;
        if (right - left == 1)
            return append({operation_(aggregate(left_node), aggregate(right_node)), 0, 0});
        int middle = left + (right - left) / 2;
        int merged_left = merge0(pool_[left_node].left, pool_[right_node].left, left, middle);
        int merged_right = merge0(pool_[left_node].right, pool_[right_node].right, middle, right);
        return append({operation_(aggregate(merged_left), aggregate(merged_right)), merged_left, merged_right});
    }

  public:
    using aggregate_type = T;
    using nseg_state_type = monostate;
    using node_view = nseg_node<npersistent_seg>;

    explicit npersistent_seg(int n = 0, O operation = {})
        : operation_(move(operation)), size_(checked_size(n)), base_(nbitceil(size_)), roots_{0} {}
    explicit npersistent_seg(domain_type domain, int n = 0, O operation = {})
        : operation_(move(operation)), size_(checked_size(n)), base_(nbitceil(size_)), pool_(move(domain)), roots_{0} {}

    npersistent_seg(const npersistent_seg& other)
        : operation_(other.operation_), size_(other.size_), base_(other.base_), pool_(other.pool_.clone()),
          roots_(other.roots_) {}
    npersistent_seg(npersistent_seg&& other) noexcept(
        is_nothrow_move_constructible_v<O> && is_nothrow_move_constructible_v<domain_type> &&
        is_nothrow_move_constructible_v<vector<int>>)
        : operation_(move(other.operation_)), size_(exchange(other.size_, 0)),
          base_(exchange(other.base_, 1)), pool_(move(other.pool_)), roots_(move(other.roots_)) {
        other.touch();
    }
    npersistent_seg& operator=(const npersistent_seg& other) {
        if (this != addressof(other)) {
            operation_ = other.operation_;
            size_ = other.size_;
            base_ = other.base_;
            pool_ = other.pool_.clone();
            roots_ = other.roots_;
        }
        touch();
        return *this;
    }
    npersistent_seg& operator=(npersistent_seg&& other) noexcept(
        is_nothrow_move_assignable_v<O> && is_nothrow_move_assignable_v<domain_type> &&
        is_nothrow_move_assignable_v<vector<int>>) {
        if (this != addressof(other)) {
            operation_ = move(other.operation_);
            size_ = exchange(other.size_, 0);
            base_ = exchange(other.base_, 1);
            pool_ = move(other.pool_);
            roots_ = move(other.roots_);
            touch();
            other.touch();
        }
        return *this;
    }

    template <nindexed V>
    explicit npersistent_seg(const V& source, O operation = {})
        : operation_(move(operation)), size_(checked_size(nlen(source))), base_(nbitceil(size_)) {
        roots_.push_back(size_ ? build(source, 0, base_) : 0);
    }
    template <nindexed V>
    explicit npersistent_seg(domain_type domain, const V& source, O operation = {})
        : operation_(move(operation)), size_(checked_size(nlen(source))), base_(nbitceil(size_)), pool_(move(domain)),
          roots_{} {
        roots_.push_back(size_ ? build(source, 0, base_) : 0);
    }

    int len() const noexcept { return size_; }
    int versions() const noexcept {
        npre(roots_.size() <= size_t(INT_MAX));
        return int(roots_.size());
    }
    int nodes() const noexcept { return pool_.len(); }
    const O& operation() const noexcept { return operation_; }
    domain_type domain() const { return pool_; }
    bool same_domain(const npersistent_seg& other) const noexcept {
        return pool_.same_domain(other.pool_);
    }

    void reserve_nodes(int count) {
        npre(count >= 0);
        pool_.reserve(count);
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

    /**
     * Create a new immutable version by combining corresponding leaves of two
     * existing versions.  The operation order is `left_value op right_value`,
     * not interval concatenation.  A missing subtree is the operation identity;
     * when one side is missing its immutable subtree is shared directly, while
     * nodes on paths where both sides exist are rebuilt.  Old versions and their
     * views therefore remain valid.  The cost is O(U) for the union of the two
     * materialized paths/subtrees and the new storage is path-copying, not a
     * destructive rewrite.
     */
    int merge(int left_version, int right_version) {
        npre(0 <= left_version && left_version < versions());
        npre(0 <= right_version && right_version < versions());
        npre(roots_.size() < size_t(INT_MAX));
        int root = merge0(roots_[size_t(left_version)], roots_[size_t(right_version)], 0, base_);
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

    node_view root(int version) const {
        npre(0 <= version && version < versions());
        return node_view(this, roots_[size_t(version)], pool_.epoch(), 0, base_);
    }
    node_view root() const { return root(0); }

  private:
    uint64_t nseg_epoch() const noexcept { return pool_.epoch(); }
    const void* nseg_domain_token() const noexcept { return pool_.domain_token(); }
    nnode_identity nseg_identity_of(int handle) const noexcept {
        return handle ? pool_.identity(handle)
                      : nnode_identity{pool_.domain_token(), 0, 0};
    }
    bool nseg_alive(int handle) const noexcept { return pool_.alive(handle); }
    T nseg_aggregate(int handle) const { return aggregate(handle); }
    int nseg_left(int handle) const noexcept { return handle ? pool_[handle].left : 0; }
    int nseg_right(int handle) const noexcept { return handle ? pool_[handle].right : 0; }
};
