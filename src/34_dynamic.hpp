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

  public:
    // Several owners may share this pool.  A copied tree clones it; a tree made
    // from an explicit domain participates in destructive structural merge.
    using domain_type = nnode_domain<node>;

  private:
    [[no_unique_address]] O operation_;
    long long domain_left_ = 0, domain_right_ = 1;
    mutable domain_type pool_;
    int root_ = 0;
    int owned_nodes_ = 0;

    friend class nseg_node<ndynamic_seg>;

    static void check_domain(long long left, long long right) {
        npre(left < right);
        npre(__int128_t(right) - left <= __int128_t(LLONG_MAX));
    }
    static long long middle(long long left, long long right) {
        return left + static_cast<long long>((__int128_t(right) - left) / 2);
    }
    void touch() noexcept {
        pool_.touch();
    }
    int make_node() {
        npre(owned_nodes_ < INT_MAX);
        int handle = pool_.make(operation_.id(), 0, 0);
        ++owned_nodes_;
        return handle;
    }
    int ensure(int& handle) {
        if (!handle)
            handle = make_node();
        return handle;
    }
    T aggregate_of(int handle) const { return handle ? pool_[handle].aggregate : operation_.id(); }
    void pull(int handle) {
        pool_[handle].aggregate = operation_(aggregate_of(pool_[handle].left),
                                             aggregate_of(pool_[handle].right));
    }
    void set0(int& handle, long long left, long long right, long long index, T value) {
        ensure(handle);
        if (right - left == 1) {
            pool_[handle].aggregate = move(value);
            return;
        }
        long long mid = middle(left, right);
        if (index < mid) {
            int child = pool_[handle].left;
            ensure(child);
            set0(child, left, mid, index, move(value));
            pool_[handle].left = child;
        } else {
            int child = pool_[handle].right;
            ensure(child);
            set0(child, mid, right, index, move(value));
            pool_[handle].right = child;
        }
        pull(handle);
    }
    void combine0(int& handle, long long left, long long right, long long index, const T& delta) {
        ensure(handle);
        if (right - left == 1) {
            pool_[handle].aggregate = operation_(move(pool_[handle].aggregate), delta);
            return;
        }
        long long mid = middle(left, right);
        if (index < mid) {
            int child = pool_[handle].left;
            combine0(child, left, mid, index, delta);
            pool_[handle].left = child;
        } else {
            int child = pool_[handle].right;
            combine0(child, mid, right, index, delta);
            pool_[handle].right = child;
        }
        pull(handle);
    }
    T fold0(int handle, long long left, long long right, long long query_left,
            long long query_right) const {
        if (!handle || query_right <= left || right <= query_left)
            return operation_.id();
        if (query_left <= left && right <= query_right)
            return pool_[handle].aggregate;
        long long mid = middle(left, right);
        return operation_(fold0(pool_[handle].left, left, mid, query_left, query_right),
                          fold0(pool_[handle].right, mid, right, query_left, query_right));
    }
    void release(int handle) {
        if (!handle)
            return;
        int left = pool_[handle].left, right = pool_[handle].right;
        release(left);
        release(right);
        pool_.erase(handle);
    }
    int count_nodes(int handle) const {
        if (!handle)
            return 0;
        return 1 + count_nodes(pool_[handle].left) + count_nodes(pool_[handle].right);
    }

    // Combine two sparse trees point by point.  This is deliberately not an
    // aggregate-level `op(left.aggregate,right.aggregate)`: for ordered or
    // otherwise non-commutative values that would concatenate whole intervals
    // instead of combining corresponding leaves.  Missing subtrees are the
    // operation identity and may be transferred without copying.
    int merge0(int left, int right, long long bound_left, long long bound_right) {
        if (!left)
            return right;
        if (!right)
            return left;
        if (bound_right - bound_left == 1) {
            pool_[left].aggregate = operation_(move(pool_[left].aggregate), pool_[right].aggregate);
            pool_.erase(right);
            return left;
        }
        long long middle_point = middle(bound_left, bound_right);
        int left_left = pool_[left].left, left_right = pool_[left].right;
        int right_left = pool_[right].left, right_right = pool_[right].right;
        int merged_left = merge0(left_left, right_left, bound_left, middle_point);
        int merged_right = merge0(left_right, right_right, middle_point, bound_right);
        pool_[left].left = merged_left;
        pool_[left].right = merged_right;
        pull(left);
        pool_.erase(right);
        return left;
    }

  public:
    using value_type = T;
    using aggregate_type = T;
    using nseg_state_type = monostate;
    using node_view = nseg_node<ndynamic_seg>;

    ndynamic_seg() = default;
    explicit ndynamic_seg(long long left, long long right, O operation = {})
        : ndynamic_seg(domain_type{}, left, right, move(operation)) {}
    explicit ndynamic_seg(domain_type domain, long long left, long long right, O operation = {})
        : operation_(move(operation)), domain_left_(left), domain_right_(right), pool_(move(domain)) {
        check_domain(left, right);
    }

    ndynamic_seg(const ndynamic_seg& other)
        : operation_(other.operation_), domain_left_(other.domain_left_), domain_right_(other.domain_right_),
          pool_(other.pool_.clone()), root_(other.root_), owned_nodes_(other.owned_nodes_) {}
    ndynamic_seg(ndynamic_seg&& other) noexcept(
        is_nothrow_move_constructible_v<O> && is_nothrow_move_constructible_v<domain_type>)
        : operation_(move(other.operation_)), domain_left_(other.domain_left_),
          domain_right_(other.domain_right_), pool_(move(other.pool_)), root_(exchange(other.root_, 0)),
          owned_nodes_(exchange(other.owned_nodes_, 0)) {
        other.touch();
    }
    ndynamic_seg& operator=(const ndynamic_seg& other) {
        if (this != addressof(other)) {
            operation_ = other.operation_;
            domain_left_ = other.domain_left_;
            domain_right_ = other.domain_right_;
            pool_ = other.pool_.clone();
            root_ = other.root_;
            owned_nodes_ = other.owned_nodes_;
        }
        touch();
        return *this;
    }
    ndynamic_seg& operator=(ndynamic_seg&& other) noexcept(
        is_nothrow_move_assignable_v<O> && is_nothrow_move_assignable_v<domain_type>) {
        if (this != addressof(other)) {
            operation_ = move(other.operation_);
            domain_left_ = other.domain_left_;
            domain_right_ = other.domain_right_;
            pool_ = move(other.pool_);
            root_ = exchange(other.root_, 0);
            owned_nodes_ = exchange(other.owned_nodes_, 0);
            touch();
            other.touch();
        }
        return *this;
    }

    long long left_bound() const noexcept { return domain_left_; }
    long long right_bound() const noexcept { return domain_right_; }
    long long width() const { return domain_right_ - domain_left_; }
    int nodes() const {
        return owned_nodes_;
    }
    bool empty() const noexcept { return root_ == 0; }
    const O& operation() const noexcept { return operation_; }
    domain_type domain() const { return pool_; }
    bool same_domain(const ndynamic_seg& other) const noexcept {
        return pool_.same_domain(other.pool_);
    }
    void reserve_nodes(int count) {
        npre(count >= 0);
        // Handles are positive, but the pool's capacity counts slots rather than
        // the zero sentinel; adding one here can overflow at INT_MAX.
        pool_.reserve(count);
    }
    void clear() {
        if (root_) {
            release(exchange(root_, 0));
            owned_nodes_ = 0;
            touch();
        }
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

    /**
     * Consume `other` and combine corresponding leaves into this sparse tree.
     * Both owners must share a domain, have equal coordinate bounds, own disjoint
     * roots, and use semantically equivalent ordered operations.  The leaf rule is
     * `this_value = op(this_value, other_value)`; it is not interval concatenation.
     * Missing subtrees are the operation identity.  The merge is destructive and
     * visits the union of materialized nodes, so its structural cost is O(U), where
     * U is the number of nodes reached by the two roots.  Old views in the shared
     * domain expire after the transaction.
     */
    void merge_from(ndynamic_seg&& other) {
        npre(this != addressof(other));
        npre(same_domain(other));
        npre(domain_left_ == other.domain_left_ && domain_right_ == other.domain_right_);
        npre(!root_ || !other.root_ || root_ != other.root_);
        root_ = merge0(root_, exchange(other.root_, 0), domain_left_, domain_right_);
        owned_nodes_ = count_nodes(root_);
        other.owned_nodes_ = 0;
        touch();
    }

    node_view root() const { return node_view(this, root_, pool_.epoch(), domain_left_, domain_right_); }

  private:
    uint64_t nseg_epoch() const noexcept { return pool_.epoch(); }
    const void* nseg_domain_token() const noexcept { return pool_.domain_token(); }
    nnode_identity nseg_identity_of(int handle) const noexcept {
        return handle ? pool_.identity(handle)
                      : nnode_identity{pool_.domain_token(), 0, 0};
    }
    bool nseg_alive(int handle) const noexcept {
        return pool_.alive(handle);
    }
    T nseg_aggregate(int handle) const { return aggregate_of(handle); }
    int nseg_left(int handle) const noexcept { return handle ? pool_[handle].left : 0; }
    int nseg_right(int handle) const noexcept { return handle ? pool_[handle].right : 0; }
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

  public:
    // The same resource domain can host disjoint sparse trees for a destructive
    // pointwise merge.  Copying an owner clones all reachable resources instead.
    using domain_type = nnode_domain<node>;

  private:
    [[no_unique_address]] M operation_;
    [[no_unique_address]] A action_;
    long long domain_left_ = 0, domain_right_ = 1;
    mutable domain_type pool_;
    int root_ = 0;
    int owned_nodes_ = 0;

    friend class nseg_node<ndynamic_lazyseg>;

    static void check_domain(long long left, long long right) {
        npre(left < right);
        npre(__int128_t(right) - left <= __int128_t(INT_MAX));
    }
    static long long middle(long long left, long long right) {
        return left + static_cast<long long>((__int128_t(right) - left) / 2);
    }
    void touch() noexcept { pool_.touch(); }
    int make_node() {
        npre(owned_nodes_ < INT_MAX);
        int handle = pool_.make(operation_.id(), 0, 0, action_.tag_id(), false);
        ++owned_nodes_;
        return handle;
    }
    int ensure(int& handle) {
        if (!handle)
            handle = make_node();
        return handle;
    }
    S aggregate_of(int handle) const { return handle ? pool_[handle].aggregate : operation_.id(); }
    void put(int handle, long long length, const F& tag) {
        node& current = pool_[handle];
        current.aggregate = action_.apply(move(current.aggregate), tag, ni::nchecked_int(length));
        if (current.pending)
            current.lazy = action_.compose(tag, current.lazy);
        else {
            current.lazy = tag;
            current.pending = true;
        }
    }
    void push(int handle, long long left, long long right) {
        if (!pool_[handle].pending || right - left == 1)
            return;
        long long mid = middle(left, right);
        F tag = pool_[handle].lazy;
        int left_child = pool_[handle].left;
        int right_child = pool_[handle].right;
        ensure(left_child);
        ensure(right_child);
        pool_[handle].left = left_child;
        pool_[handle].right = right_child;
        put(left_child, mid - left, tag);
        put(right_child, right - mid, tag);
        pool_[handle].lazy = action_.tag_id();
        pool_[handle].pending = false;
    }
    void pull(int handle) {
        node& current = pool_[handle];
        current.aggregate = operation_(aggregate_of(current.left), aggregate_of(current.right));
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
        int left_child = pool_[handle].left;
        int right_child = pool_[handle].right;
        apply0(left_child, left, mid, query_left, query_right, tag);
        apply0(right_child, mid, right, query_left, query_right, tag);
        pool_[handle].left = left_child;
        pool_[handle].right = right_child;
        pull(handle);
    }
    S fold0(int handle, long long left, long long right, long long query_left,
            long long query_right, F carry, bool carried) const {
        if (query_right <= left || right <= query_left || (!handle && !carried))
            return operation_.id();
        if (query_left <= left && right <= query_right) {
            S result = aggregate_of(handle);
            return carried ? action_.apply(move(result), carry, ni::nchecked_int(right - left))
                           : result;
        }
        if (handle && pool_[handle].pending) {
            carry = carried ? action_.compose(carry, pool_[handle].lazy) : pool_[handle].lazy;
            carried = true;
        }
        long long mid = middle(left, right);
        int left_child = handle ? pool_[handle].left : 0;
        int right_child = handle ? pool_[handle].right : 0;
        return operation_(fold0(left_child, left, mid, query_left, query_right, carry, carried),
                          fold0(right_child, mid, right, query_left, query_right, carry, carried));
    }
    void set0(int handle, long long left, long long right, long long index, S value) {
        if (right - left == 1) {
            node& current = pool_[handle];
            current.aggregate = move(value);
            current.lazy = action_.tag_id();
            current.pending = false;
            return;
        }
        push(handle, left, right);
        long long mid = middle(left, right);
        if (index < mid) {
            int child = pool_[handle].left;
            ensure(child);
            set0(child, left, mid, index, move(value));
            pool_[handle].left = child;
        } else {
            int child = pool_[handle].right;
            ensure(child);
            set0(child, mid, right, index, move(value));
            pool_[handle].right = child;
        }
        pull(handle);
    }
    void release(int handle) {
        if (!handle)
            return;
        int left = pool_[handle].left, right = pool_[handle].right;
        release(left);
        release(right);
        pool_.erase(handle);
    }
    int count_nodes(int handle) const {
        if (!handle)
            return 0;
        return 1 + count_nodes(pool_[handle].left) + count_nodes(pool_[handle].right);
    }

    // Both pending tags are pushed before descending.  This makes the leaf rule
    // unambiguous even for non-commutative actions: each aggregate is first put in
    // its logical state, then corresponding values are merged left before right.
    int merge0(int left_node, int right_node, long long left, long long right) {
        if (!left_node)
            return right_node;
        if (!right_node)
            return left_node;
        if (right - left == 1) {
            pool_[left_node].aggregate =
                operation_(move(pool_[left_node].aggregate), pool_[right_node].aggregate);
            pool_[left_node].lazy = action_.tag_id();
            pool_[left_node].pending = false;
            pool_.erase(right_node);
            return left_node;
        }
        push(left_node, left, right);
        push(right_node, left, right);
        long long mid = middle(left, right);
        int merged_left = merge0(pool_[left_node].left, pool_[right_node].left, left, mid);
        int merged_right = merge0(pool_[left_node].right, pool_[right_node].right, mid, right);
        pool_[left_node].left = merged_left;
        pool_[left_node].right = merged_right;
        pull(left_node);
        pool_.erase(right_node);
        return left_node;
    }

  public:
    using value_type = S;
    using aggregate_type = S;
    using tag_type = F;
    using nseg_state_type = optional<F>;
    using node_view = nseg_node<ndynamic_lazyseg>;

    ndynamic_lazyseg() = default;
    explicit ndynamic_lazyseg(long long left, long long right, M operation = {}, A action = {})
        : ndynamic_lazyseg(domain_type{}, left, right, move(operation), move(action)) {}
    explicit ndynamic_lazyseg(domain_type domain, long long left, long long right, M operation = {}, A action = {})
        : operation_(move(operation)), action_(move(action)), domain_left_(left), domain_right_(right),
          pool_(move(domain)) {
        check_domain(left, right);
    }

    ndynamic_lazyseg(const ndynamic_lazyseg& other)
        : operation_(other.operation_), action_(other.action_), domain_left_(other.domain_left_),
          domain_right_(other.domain_right_), pool_(other.pool_.clone()), root_(other.root_),
          owned_nodes_(other.owned_nodes_) {}
    ndynamic_lazyseg(ndynamic_lazyseg&& other) noexcept(
        is_nothrow_move_constructible_v<M> && is_nothrow_move_constructible_v<A> &&
        is_nothrow_move_constructible_v<domain_type>)
        : operation_(move(other.operation_)), action_(move(other.action_)),
          domain_left_(other.domain_left_), domain_right_(other.domain_right_), pool_(move(other.pool_)),
          root_(exchange(other.root_, 0)), owned_nodes_(exchange(other.owned_nodes_, 0)) {
        other.touch();
    }
    ndynamic_lazyseg& operator=(const ndynamic_lazyseg& other) {
        if (this != addressof(other)) {
            operation_ = other.operation_;
            action_ = other.action_;
            domain_left_ = other.domain_left_;
            domain_right_ = other.domain_right_;
            pool_ = other.pool_.clone();
            root_ = other.root_;
            owned_nodes_ = other.owned_nodes_;
        }
        touch();
        return *this;
    }
    ndynamic_lazyseg& operator=(ndynamic_lazyseg&& other) noexcept(
        is_nothrow_move_assignable_v<M> && is_nothrow_move_assignable_v<A> &&
        is_nothrow_move_assignable_v<domain_type>) {
        if (this != addressof(other)) {
            operation_ = move(other.operation_);
            action_ = move(other.action_);
            domain_left_ = other.domain_left_;
            domain_right_ = other.domain_right_;
            pool_ = move(other.pool_);
            root_ = exchange(other.root_, 0);
            owned_nodes_ = exchange(other.owned_nodes_, 0);
            touch();
            other.touch();
        }
        return *this;
    }

    long long left_bound() const noexcept { return domain_left_; }
    long long right_bound() const noexcept { return domain_right_; }
    long long width() const { return domain_right_ - domain_left_; }
    int nodes() const noexcept { return owned_nodes_; }
    bool empty() const noexcept { return root_ == 0; }
    const M& operation() const noexcept { return operation_; }
    const A& action() const noexcept { return action_; }
    domain_type domain() const { return pool_; }
    bool same_domain(const ndynamic_lazyseg& other) const noexcept {
        return pool_.same_domain(other.pool_);
    }
    void reserve_nodes(int count) {
        npre(count >= 0);
        pool_.reserve(count);
    }
    void clear() {
        release(exchange(root_, 0));
        owned_nodes_ = 0;
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

    /**
     * Destructively combine corresponding leaves of two sparse lazy trees.  The
     * owners must share a domain, have equal bounds and disjoint roots, and use
     * equivalent merge/action semantics.  Pending tags are pushed only because
     * both roots are present on the same interval; a missing subtree transfers
     * without materializing it.  Thus the cost is O(U), with U the union of the
     * materialized structure (plus children needed to expose pending tags).
     */
    void merge_from(ndynamic_lazyseg&& other) {
        npre(this != addressof(other));
        npre(same_domain(other));
        npre(domain_left_ == other.domain_left_ && domain_right_ == other.domain_right_);
        npre(!root_ || !other.root_ || root_ != other.root_);
        root_ = merge0(root_, exchange(other.root_, 0), domain_left_, domain_right_);
        owned_nodes_ = count_nodes(root_);
        other.owned_nodes_ = 0;
        touch();
    }

    node_view root() const { return node_view(this, root_, pool_.epoch(), domain_left_, domain_right_); }

  private:
    uint64_t nseg_epoch() const noexcept { return pool_.epoch(); }
    const void* nseg_domain_token() const noexcept { return pool_.domain_token(); }
    nnode_identity nseg_identity_of(int handle) const noexcept {
        return handle ? pool_.identity(handle)
                      : nnode_identity{pool_.domain_token(), 0, 0};
    }
    bool nseg_alive(int handle) const noexcept { return pool_.alive(handle); }
    S nseg_aggregate(int handle) const { return aggregate_of(handle); }
    int nseg_left(int handle) const noexcept { return handle ? pool_[handle].left : 0; }
    int nseg_right(int handle) const noexcept { return handle ? pool_[handle].right : 0; }
    F nseg_tag(int handle) const {
        return handle && pool_[handle].pending ? pool_[handle].lazy : action_.tag_id();
    }
    bool nseg_pending(int handle) const noexcept {
        return handle && pool_[handle].pending;
    }
    F nseg_compose(const F& newer, const F& older) const { return action_.compose(newer, older); }
    S nseg_apply(S aggregate, const F& tag, int length) const {
        return action_.apply(move(aggregate), tag, length);
    }
};

template <class T> using ndynamic_addsum = ndynamic_lazyseg<T, T, nadd<T>, naddsum_action<T>>;
