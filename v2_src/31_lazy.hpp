template <class S, class F, class M, class A>
    requires nmonoid<M, S> && naction<A, S, F>
class nlazyseg {
    [[no_unique_address]] M operation_;
    [[no_unique_address]] A action_;
    int size_ = 0, base_ = 1;
    vector<S> tree_;
    vector<F> lazy_;
    vector<unsigned char> pending_;

    static int checked_size(int n) {
        npre(0 <= n && n <= (1 << 30));
        return n;
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

    S fold0(int node, int left, int right, int query_left, int query_right) {
        if (query_left <= left && right <= query_right)
            return tree_[node];
        push(node, left, right);
        int middle = left + (right - left) / 2;
        if (query_right <= middle)
            return fold0(node << 1, left, middle, query_left, query_right);
        if (middle <= query_left)
            return fold0(node << 1 | 1, middle, right, query_left, query_right);
        return operation_(fold0(node << 1, left, middle, query_left, query_right),
                          fold0(node << 1 | 1, middle, right, query_left, query_right));
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
    nlazyseg()
        : tree_(2, operation_.id()), lazy_(2, action_.tag_id()), pending_(2, false) {}

    explicit nlazyseg(int n, M operation = {}, A action = {})
        : operation_(move(operation)), action_(move(action)), size_(checked_size(n)), base_(nbitceil(size_)),
          tree_(size_t(2) * base_, operation_.id()), lazy_(size_t(2) * base_, action_.tag_id()),
          pending_(size_t(2) * base_, false) {}

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
    }

    void apply(int left, int right, const F& tag) {
        npre(0 <= left && left <= right && right <= size_);
        if (left < right)
            apply0(1, 0, base_, left, right, tag);
    }

    S fold(int left, int right) {
        npre(0 <= left && left <= right && right <= size_);
        return left == right ? operation_.id() : fold0(1, 0, base_, left, right);
    }
    S fold() const { return size_ ? tree_[1] : operation_.id(); }

    void set(int index, S value) {
        npre(0 <= index && index < size_);
        set0(1, 0, base_, index, move(value));
    }
    S get(int index) {
        npre(0 <= index && index < size_);
        return fold(index, index + 1);
    }
};

template <class T> using nlazy_addsum = nlazyseg<T, T, nadd<T>, naddsum_action<T>>;
