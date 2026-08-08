template <class T> struct naddsum_action {
    constexpr T tag_id() const { return T{}; }
    constexpr T compose(const T& newer, const T& older) const { return older + newer; }
    constexpr T apply(T sum, const T& delta, int length) const { return sum + delta * T(length); }
};

template <class S, class F, class M, class A>
class nlazyseg {
    [[no_unique_address]] M operation_;
    [[no_unique_address]] A action_;
    int size_ = 0, base_ = 1;
    vector<S> tree_;
    vector<F> lazy_;
    vector<unsigned char> pending_;
    uint64_t epoch_ = 1;

    friend class nseg_node<nlazyseg>;

    static int checked_size(int n) {
        npre(0 <= n && n <= (1 << 30));
        return n;
    }

    void touch() noexcept {
        if (!++epoch_)
            ++epoch_;
    }

    void put(int node, int length, const F& tag) {
        tree_[node] = action_.apply(move(tree_[node]), tag, length);
        if (pending_[node])
            lazy_[node] = action_.compose(tag, lazy_[node]);
        else {
            lazy_[node] = tag;
            pending_[node] = true;
        }
    }

    void push(int node, int left, int right) {
        if (!pending_[node] || right - left == 1)
            return;
        int middle = left + (right - left) / 2;
        put(node << 1, middle - left, lazy_[node]);
        put(node << 1 | 1, right - middle, lazy_[node]);
        lazy_[node] = action_.tag_id();
        pending_[node] = false;
    }

    void pull(int node) { tree_[node] = operation_(tree_[node << 1], tree_[node << 1 | 1]); }

    void apply0(int node, int left, int right, int query_left, int query_right, const F& tag) {
        if (query_left <= left && right <= query_right) {
            put(node, right - left, tag);
            return;
        }
        push(node, left, right);
        int middle = left + (right - left) / 2;
        if (query_left < middle)
            apply0(node << 1, left, middle, query_left, query_right, tag);
        if (middle < query_right)
            apply0(node << 1 | 1, middle, right, query_left, query_right, tag);
        pull(node);
    }

    S fold0(int node, int left, int right, int query_left, int query_right, F carry,
            bool carried) const {
        if (query_left <= left && right <= query_right) {
            S result = tree_[node];
            return carried ? action_.apply(move(result), carry, right - left) : result;
        }
        if (pending_[node]) {
            carry = carried ? action_.compose(carry, lazy_[node]) : lazy_[node];
            carried = true;
        }
        int middle = left + (right - left) / 2;
        if (query_right <= middle)
            return fold0(node << 1, left, middle, query_left, query_right, move(carry), carried);
        if (middle <= query_left)
            return fold0(node << 1 | 1, middle, right, query_left, query_right, move(carry), carried);
        return operation_(fold0(node << 1, left, middle, query_left, query_right, carry, carried),
                          fold0(node << 1 | 1, middle, right, query_left, query_right, move(carry),
                                carried));
    }

    void set0(int node, int left, int right, int index, S value) {
        if (right - left == 1) {
            tree_[node] = move(value);
            lazy_[node] = action_.tag_id();
            pending_[node] = false;
            return;
        }
        push(node, left, right);
        int middle = left + (right - left) / 2;
        if (index < middle)
            set0(node << 1, left, middle, index, move(value));
        else
            set0(node << 1 | 1, middle, right, index, move(value));
        pull(node);
    }

  public:
    using aggregate_type = S;
    using tag_type = F;
    using nseg_state_type = optional<F>;
    using node_view = nseg_node<nlazyseg>;

    nlazyseg()
        : tree_(2, operation_.id()), lazy_(2, action_.tag_id()), pending_(2, false) {}

    explicit nlazyseg(int n, M operation = {}, A action = {})
        : operation_(move(operation)), action_(move(action)), size_(checked_size(n)), base_(nbitceil(size_)),
          tree_(size_t(2) * base_, operation_.id()), lazy_(size_t(2) * base_, action_.tag_id()),
          pending_(size_t(2) * base_, false) {}

    nlazyseg(const nlazyseg& other)
        : operation_(other.operation_), action_(other.action_), size_(other.size_), base_(other.base_),
          tree_(other.tree_), lazy_(other.lazy_), pending_(other.pending_) {}
    nlazyseg(nlazyseg&& other) noexcept(
        is_nothrow_move_constructible_v<M> && is_nothrow_move_constructible_v<A> &&
        is_nothrow_move_constructible_v<vector<S>> && is_nothrow_move_constructible_v<vector<F>> &&
        is_nothrow_move_constructible_v<vector<unsigned char>>)
        : operation_(move(other.operation_)), action_(move(other.action_)),
          size_(exchange(other.size_, 0)), base_(exchange(other.base_, 1)), tree_(move(other.tree_)),
          lazy_(move(other.lazy_)), pending_(move(other.pending_)) {
        other.touch();
    }
    nlazyseg& operator=(const nlazyseg& other) {
        if (this != addressof(other)) {
            operation_ = other.operation_;
            action_ = other.action_;
            size_ = other.size_;
            base_ = other.base_;
            tree_ = other.tree_;
            lazy_ = other.lazy_;
            pending_ = other.pending_;
            touch();
        }
        return *this;
    }
    nlazyseg& operator=(nlazyseg&& other) noexcept(
        is_nothrow_move_assignable_v<M> && is_nothrow_move_assignable_v<A> &&
        is_nothrow_move_assignable_v<vector<S>> && is_nothrow_move_assignable_v<vector<F>> &&
        is_nothrow_move_assignable_v<vector<unsigned char>>) {
        if (this != addressof(other)) {
            operation_ = move(other.operation_);
            action_ = move(other.action_);
            size_ = exchange(other.size_, 0);
            base_ = exchange(other.base_, 1);
            tree_ = move(other.tree_);
            lazy_ = move(other.lazy_);
            pending_ = move(other.pending_);
            touch();
            other.touch();
        }
        return *this;
    }

    template <nindexed V>
    explicit nlazyseg(const V& source, M operation = {}, A action = {})
        : nlazyseg(nlen(source), move(operation), move(action)) {
        for (int i = 0; i < size_; ++i)
            tree_[base_ + i] = source[i];
        for (int i = base_; --i > 0;)
            pull(i);
    }

    int len() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    const M& operation() const noexcept { return operation_; }
    const A& action() const noexcept { return action_; }

    void clear() {
        fill(tree_.begin(), tree_.end(), operation_.id());
        fill(lazy_.begin(), lazy_.end(), action_.tag_id());
        fill(pending_.begin(), pending_.end(), false);
        touch();
    }

    void apply(int left, int right, const F& tag) {
        npre(0 <= left && left <= right && right <= size_);
        if (left < right) {
            apply0(1, 0, base_, left, right, tag);
            touch();
        }
    }

    S fold(int left, int right) const {
        npre(0 <= left && left <= right && right <= size_);
        return left == right
                   ? operation_.id()
                   : fold0(1, 0, base_, left, right, action_.tag_id(), false);
    }
    S fold() const { return size_ ? tree_[1] : operation_.id(); }

    void set(int index, S value) {
        npre(0 <= index && index < size_);
        set0(1, 0, base_, index, move(value));
        touch();
    }
    S get(int index) const {
        npre(0 <= index && index < size_);
        return fold(index, index + 1);
    }

    node_view root() const { return node_view(this, size_ ? 1 : 0, epoch_, 0, base_); }

  private:
    uint64_t nseg_epoch() const noexcept { return epoch_; }
    bool nseg_alive(int handle) const noexcept {
        return 0 < handle && size_t(handle) < tree_.size();
    }
    S nseg_aggregate(int handle) const { return handle ? tree_[size_t(handle)] : operation_.id(); }
    int nseg_left(int handle) const noexcept {
        return handle && size_t(2LL * handle) < tree_.size() ? 2 * handle : 0;
    }
    int nseg_right(int handle) const noexcept {
        return handle && size_t(2LL * handle + 1) < tree_.size() ? 2 * handle + 1 : 0;
    }
    F nseg_tag(int handle) const {
        return handle && pending_[size_t(handle)] ? lazy_[size_t(handle)] : action_.tag_id();
    }
    bool nseg_pending(int handle) const noexcept {
        return handle && pending_[size_t(handle)];
    }
    F nseg_compose(const F& newer, const F& older) const { return action_.compose(newer, older); }
    S nseg_apply(S aggregate, const F& tag, int length) const {
        return action_.apply(move(aggregate), tag, length);
    }
};

template <class T> using nlazy_addsum = nlazyseg<T, T, nadd<T>, naddsum_action<T>>;
