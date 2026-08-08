// Dynamic (open-point) segment trees.  Nodes are allocated only on paths that
// are written; absent children represent the operation identity.

/**
 * Sparse point-update segment tree on the fixed half-open long-long domain [lo,hi).
 * O has a two-sided identity and associative ordered merge.  Domain width must fit
 * signed long long; each write allocates O(log(width)) nodes, reads allocate none.
 * Any mutation invalidates structural node views.
 */
template <class T, class O = nadd<T>> class ndynamic_seg {
    struct node {
        T aggregate;
        int left = 0, right = 0;
    };

    [[no_unique_address]] O operation_;
    long long domain_left_ = 0, domain_right_ = 1;
    vector<node> nodes_{{operation_.id(), 0, 0}};
    int root_ = 0;
    uint64_t epoch_ = 1;

    friend class nseg_node<ndynamic_seg>;

    static void check_domain(long long left, long long right) {
        npre(left < right);
        npre(__int128_t(right) - left <= __int128_t(LLONG_MAX));
    }
    static long long middle(long long left, long long right) {
        return left + static_cast<long long>((__int128_t(right) - left) / 2);
    }
    void touch() noexcept {
        if (!++epoch_)
            ++epoch_;
    }
    int make_node() {
        npre(nodes_.size() < size_t(INT_MAX));
        nodes_.push_back({operation_.id(), 0, 0});
        return int(nodes_.size() - 1);
    }
    int ensure(int& handle) {
        if (!handle)
            handle = make_node();
        return handle;
    }
    void pull(int handle) {
        nodes_[size_t(handle)].aggregate =
            operation_(nodes_[size_t(nodes_[size_t(handle)].left)].aggregate,
                       nodes_[size_t(nodes_[size_t(handle)].right)].aggregate);
    }
    void set0(int& handle, long long left, long long right, long long index, T value) {
        ensure(handle);
        if (right - left == 1) {
            nodes_[size_t(handle)].aggregate = move(value);
            return;
        }
        long long mid = middle(left, right);
        if (index < mid) {
            int child = nodes_[size_t(handle)].left;
            ensure(child);
            set0(child, left, mid, index, move(value));
            nodes_[size_t(handle)].left = child;
        } else {
            int child = nodes_[size_t(handle)].right;
            ensure(child);
            set0(child, mid, right, index, move(value));
            nodes_[size_t(handle)].right = child;
        }
        pull(handle);
    }
    void combine0(int& handle, long long left, long long right, long long index, const T& delta) {
        ensure(handle);
        if (right - left == 1) {
            nodes_[size_t(handle)].aggregate = operation_(move(nodes_[size_t(handle)].aggregate), delta);
            return;
        }
        long long mid = middle(left, right);
        if (index < mid) {
            int child = nodes_[size_t(handle)].left;
            combine0(child, left, mid, index, delta);
            nodes_[size_t(handle)].left = child;
        } else {
            int child = nodes_[size_t(handle)].right;
            combine0(child, mid, right, index, delta);
            nodes_[size_t(handle)].right = child;
        }
        pull(handle);
    }
    T fold0(int handle, long long left, long long right, long long query_left,
            long long query_right) const {
        if (!handle || query_right <= left || right <= query_left)
            return operation_.id();
        if (query_left <= left && right <= query_right)
            return nodes_[size_t(handle)].aggregate;
        long long mid = middle(left, right);
        return operation_(fold0(nodes_[size_t(handle)].left, left, mid, query_left, query_right),
                          fold0(nodes_[size_t(handle)].right, mid, right, query_left, query_right));
    }

  public:
    using value_type = T;
    using aggregate_type = T;
    using nseg_state_type = monostate;
    using node_view = nseg_node<ndynamic_seg>;

    ndynamic_seg() = default;
    explicit ndynamic_seg(long long left, long long right, O operation = {})
        : operation_(move(operation)), domain_left_(left), domain_right_(right),
          nodes_{{operation_.id(), 0, 0}} {
        check_domain(left, right);
    }

    ndynamic_seg(const ndynamic_seg& other)
        : operation_(other.operation_), domain_left_(other.domain_left_),
          domain_right_(other.domain_right_), nodes_(other.nodes_), root_(other.root_) {}
    ndynamic_seg(ndynamic_seg&& other) noexcept(
        is_nothrow_move_constructible_v<O> && is_nothrow_move_constructible_v<vector<node>>)
        : operation_(move(other.operation_)), domain_left_(other.domain_left_),
          domain_right_(other.domain_right_), nodes_(move(other.nodes_)), root_(exchange(other.root_, 0)) {
        other.touch();
    }
    ndynamic_seg& operator=(const ndynamic_seg& other) {
        if (this != addressof(other)) {
            operation_ = other.operation_;
            domain_left_ = other.domain_left_;
            domain_right_ = other.domain_right_;
            nodes_ = other.nodes_;
            root_ = other.root_;
            touch();
        }
        return *this;
    }
    ndynamic_seg& operator=(ndynamic_seg&& other) noexcept(
        is_nothrow_move_assignable_v<O> && is_nothrow_move_assignable_v<vector<node>>) {
        if (this != addressof(other)) {
            operation_ = move(other.operation_);
            domain_left_ = other.domain_left_;
            domain_right_ = other.domain_right_;
            nodes_ = move(other.nodes_);
            root_ = exchange(other.root_, 0);
            touch();
            other.touch();
        }
        return *this;
    }

    long long left_bound() const noexcept { return domain_left_; }
    long long right_bound() const noexcept { return domain_right_; }
    long long width() const { return domain_right_ - domain_left_; }
    int nodes() const {
        npre(nodes_.size() <= size_t(INT_MAX));
        return int(nodes_.size() - 1);
    }
    bool empty() const noexcept { return root_ == 0; }
    const O& operation() const noexcept { return operation_; }
    void reserve_nodes(int count) {
        npre(count >= 0);
        nodes_.reserve(size_t(count) + 1);
    }
    void clear() {
        nodes_.clear();
        nodes_.push_back({operation_.id(), 0, 0});
        root_ = 0;
        touch();
    }

    void set(long long index, T value) {
        npre(domain_left_ <= index && index < domain_right_);
        set0(root_, domain_left_, domain_right_, index, move(value));
        touch();
    }
    void combine(long long index, const T& delta) {
        npre(domain_left_ <= index && index < domain_right_);
        combine0(root_, domain_left_, domain_right_, index, delta);
        touch();
    }
    T fold(long long left, long long right) const {
        npre(domain_left_ <= left && left <= right && right <= domain_right_);
        return left == right ? operation_.id() : fold0(root_, domain_left_, domain_right_, left, right);
    }
    T fold() const { return fold(domain_left_, domain_right_); }
    T get(long long index) const {
        npre(domain_left_ <= index && index < domain_right_);
        return fold(index, index + 1);
    }

    node_view root() const { return node_view(this, root_, epoch_, domain_left_, domain_right_); }

  private:
    uint64_t nseg_epoch() const noexcept { return epoch_; }
    bool nseg_alive(int handle) const noexcept {
        return 0 < handle && size_t(handle) < nodes_.size();
    }
    T nseg_aggregate(int handle) const { return handle ? nodes_[size_t(handle)].aggregate : operation_.id(); }
    int nseg_left(int handle) const noexcept { return handle ? nodes_[size_t(handle)].left : 0; }
    int nseg_right(int handle) const noexcept { return handle ? nodes_[size_t(handle)].right : 0; }
};

/**
 * Sparse lazy segment tree on [lo,hi).
 * M/A obey the same merge/action contracts as nlazyseg, including
 * compose(newer,older).  Width is limited to INT_MAX because action lengths are int.
 * Updates and queries are O(log(width)); pushing a pending tag may allocate both
 * children, while read-only queries and nseg_node descent never allocate.
 */
template <class S, class F, class M, class A> class ndynamic_lazyseg {
    struct node {
        S aggregate;
        int left = 0, right = 0;
        F lazy;
        bool pending = false;
    };

    [[no_unique_address]] M operation_;
    [[no_unique_address]] A action_;
    long long domain_left_ = 0, domain_right_ = 1;
    vector<node> nodes_;
    int root_ = 0;
    uint64_t epoch_ = 1;

    friend class nseg_node<ndynamic_lazyseg>;

    static void check_domain(long long left, long long right) {
        npre(left < right);
        npre(__int128_t(right) - left <= __int128_t(INT_MAX));
    }
    static long long middle(long long left, long long right) {
        return left + static_cast<long long>((__int128_t(right) - left) / 2);
    }
    void touch() noexcept {
        if (!++epoch_)
            ++epoch_;
    }
    int make_node() {
        npre(nodes_.size() < size_t(INT_MAX));
        nodes_.push_back({operation_.id(), 0, 0, action_.tag_id(), false});
        return int(nodes_.size() - 1);
    }
    int ensure(int& handle) {
        if (!handle)
            handle = make_node();
        return handle;
    }
    void put(int handle, long long length, const F& tag) {
        node& current = nodes_[size_t(handle)];
        current.aggregate = action_.apply(move(current.aggregate), tag, ni::nchecked_int(length));
        if (current.pending)
            current.lazy = action_.compose(tag, current.lazy);
        else {
            current.lazy = tag;
            current.pending = true;
        }
    }
    void push(int handle, long long left, long long right) {
        if (!nodes_[size_t(handle)].pending || right - left == 1)
            return;
        long long mid = middle(left, right);
        F tag = nodes_[size_t(handle)].lazy;
        int left_child = nodes_[size_t(handle)].left;
        int right_child = nodes_[size_t(handle)].right;
        ensure(left_child);
        ensure(right_child);
        nodes_[size_t(handle)].left = left_child;
        nodes_[size_t(handle)].right = right_child;
        put(left_child, mid - left, tag);
        put(right_child, right - mid, tag);
        nodes_[size_t(handle)].lazy = action_.tag_id();
        nodes_[size_t(handle)].pending = false;
    }
    void pull(int handle) {
        node& current = nodes_[size_t(handle)];
        current.aggregate =
            operation_(nodes_[size_t(current.left)].aggregate, nodes_[size_t(current.right)].aggregate);
    }
    void apply0(int& handle, long long left, long long right, long long query_left,
                long long query_right, const F& tag) {
        if (query_right <= left || right <= query_left)
            return;
        ensure(handle);
        if (query_left <= left && right <= query_right) {
            put(handle, right - left, tag);
            return;
        }
        push(handle, left, right);
        long long mid = middle(left, right);
        int left_child = nodes_[size_t(handle)].left;
        int right_child = nodes_[size_t(handle)].right;
        apply0(left_child, left, mid, query_left, query_right, tag);
        apply0(right_child, mid, right, query_left, query_right, tag);
        nodes_[size_t(handle)].left = left_child;
        nodes_[size_t(handle)].right = right_child;
        pull(handle);
    }
    S fold0(int handle, long long left, long long right, long long query_left,
            long long query_right, F carry, bool carried) const {
        if (query_right <= left || right <= query_left || (!handle && !carried))
            return operation_.id();
        if (query_left <= left && right <= query_right) {
            S result = handle ? nodes_[size_t(handle)].aggregate : operation_.id();
            return carried ? action_.apply(move(result), carry, ni::nchecked_int(right - left))
                           : result;
        }
        if (handle && nodes_[size_t(handle)].pending) {
            carry = carried ? action_.compose(carry, nodes_[size_t(handle)].lazy)
                            : nodes_[size_t(handle)].lazy;
            carried = true;
        }
        long long mid = middle(left, right);
        int left_child = handle ? nodes_[size_t(handle)].left : 0;
        int right_child = handle ? nodes_[size_t(handle)].right : 0;
        return operation_(fold0(left_child, left, mid, query_left, query_right, carry, carried),
                          fold0(right_child, mid, right, query_left, query_right, carry, carried));
    }
    void set0(int handle, long long left, long long right, long long index, S value) {
        if (right - left == 1) {
            node& current = nodes_[size_t(handle)];
            current.aggregate = move(value);
            current.lazy = action_.tag_id();
            current.pending = false;
            return;
        }
        push(handle, left, right);
        long long mid = middle(left, right);
        if (index < mid) {
            int child = nodes_[size_t(handle)].left;
            ensure(child);
            set0(child, left, mid, index, move(value));
            nodes_[size_t(handle)].left = child;
        } else {
            int child = nodes_[size_t(handle)].right;
            ensure(child);
            set0(child, mid, right, index, move(value));
            nodes_[size_t(handle)].right = child;
        }
        pull(handle);
    }

  public:
    using value_type = S;
    using aggregate_type = S;
    using tag_type = F;
    using nseg_state_type = optional<F>;
    using node_view = nseg_node<ndynamic_lazyseg>;

    ndynamic_lazyseg()
        : nodes_{{operation_.id(), 0, 0, action_.tag_id(), false}} {}
    explicit ndynamic_lazyseg(long long left, long long right, M operation = {}, A action = {})
        : operation_(move(operation)), action_(move(action)), domain_left_(left), domain_right_(right),
          nodes_{{operation_.id(), 0, 0, action_.tag_id(), false}} {
        check_domain(left, right);
    }

    ndynamic_lazyseg(const ndynamic_lazyseg& other)
        : operation_(other.operation_), action_(other.action_), domain_left_(other.domain_left_),
          domain_right_(other.domain_right_), nodes_(other.nodes_), root_(other.root_) {}
    ndynamic_lazyseg(ndynamic_lazyseg&& other) noexcept(
        is_nothrow_move_constructible_v<M> && is_nothrow_move_constructible_v<A> &&
        is_nothrow_move_constructible_v<vector<node>>)
        : operation_(move(other.operation_)), action_(move(other.action_)),
          domain_left_(other.domain_left_), domain_right_(other.domain_right_), nodes_(move(other.nodes_)),
          root_(exchange(other.root_, 0)) {
        other.touch();
    }
    ndynamic_lazyseg& operator=(const ndynamic_lazyseg& other) {
        if (this != addressof(other)) {
            operation_ = other.operation_;
            action_ = other.action_;
            domain_left_ = other.domain_left_;
            domain_right_ = other.domain_right_;
            nodes_ = other.nodes_;
            root_ = other.root_;
            touch();
        }
        return *this;
    }
    ndynamic_lazyseg& operator=(ndynamic_lazyseg&& other) noexcept(
        is_nothrow_move_assignable_v<M> && is_nothrow_move_assignable_v<A> &&
        is_nothrow_move_assignable_v<vector<node>>) {
        if (this != addressof(other)) {
            operation_ = move(other.operation_);
            action_ = move(other.action_);
            domain_left_ = other.domain_left_;
            domain_right_ = other.domain_right_;
            nodes_ = move(other.nodes_);
            root_ = exchange(other.root_, 0);
            touch();
            other.touch();
        }
        return *this;
    }

    long long left_bound() const noexcept { return domain_left_; }
    long long right_bound() const noexcept { return domain_right_; }
    long long width() const { return domain_right_ - domain_left_; }
    int nodes() const {
        npre(nodes_.size() <= size_t(INT_MAX));
        return int(nodes_.size() - 1);
    }
    bool empty() const noexcept { return root_ == 0; }
    const M& operation() const noexcept { return operation_; }
    const A& action() const noexcept { return action_; }
    void reserve_nodes(int count) {
        npre(count >= 0);
        nodes_.reserve(size_t(count) + 1);
    }
    void clear() {
        nodes_.clear();
        nodes_.push_back({operation_.id(), 0, 0, action_.tag_id(), false});
        root_ = 0;
        touch();
    }

    void apply(long long left, long long right, const F& tag) {
        npre(domain_left_ <= left && left <= right && right <= domain_right_);
        if (left < right) {
            apply0(root_, domain_left_, domain_right_, left, right, tag);
            touch();
        }
    }
    S fold(long long left, long long right) const {
        npre(domain_left_ <= left && left <= right && right <= domain_right_);
        return left == right
                   ? operation_.id()
                   : fold0(root_, domain_left_, domain_right_, left, right, action_.tag_id(), false);
    }
    S fold() const { return fold(domain_left_, domain_right_); }
    void set(long long index, S value) {
        npre(domain_left_ <= index && index < domain_right_);
        ensure(root_);
        set0(root_, domain_left_, domain_right_, index, move(value));
        touch();
    }
    S get(long long index) const {
        npre(domain_left_ <= index && index < domain_right_);
        return fold(index, index + 1);
    }

    node_view root() const { return node_view(this, root_, epoch_, domain_left_, domain_right_); }

  private:
    uint64_t nseg_epoch() const noexcept { return epoch_; }
    bool nseg_alive(int handle) const noexcept {
        return 0 < handle && size_t(handle) < nodes_.size();
    }
    S nseg_aggregate(int handle) const { return handle ? nodes_[size_t(handle)].aggregate : operation_.id(); }
    int nseg_left(int handle) const noexcept { return handle ? nodes_[size_t(handle)].left : 0; }
    int nseg_right(int handle) const noexcept { return handle ? nodes_[size_t(handle)].right : 0; }
    F nseg_tag(int handle) const {
        return handle && nodes_[size_t(handle)].pending ? nodes_[size_t(handle)].lazy : action_.tag_id();
    }
    bool nseg_pending(int handle) const noexcept {
        return handle && nodes_[size_t(handle)].pending;
    }
    F nseg_compose(const F& newer, const F& older) const { return action_.compose(newer, older); }
    S nseg_apply(S aggregate, const F& tag, int length) const {
        return action_.apply(move(aggregate), tag, length);
    }
};

template <class T> using ndynamic_addsum = ndynamic_lazyseg<T, T, nadd<T>, naddsum_action<T>>;
