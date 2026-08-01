template <class T, class O = nadd<T>>
    requires ncommutative_monoid<O, T>
class nfenwick {
    [[no_unique_address]] O operation_;
    int size_ = 0;
    vector<T> tree_;

    static int checked_size(int n) {
        npre(0 <= n && n < INT_MAX);
        return n;
    }

  public:
    nfenwick() : tree_(1, operation_.id()) {}
    explicit nfenwick(int n, O operation = {})
        : operation_(move(operation)), size_(checked_size(n)), tree_(size_t(size_) + 1, operation_.id()) {}

    template <nindexed A>
    explicit nfenwick(const A& source, O operation = {}) : nfenwick(nlen(source), move(operation)) {
        for (int i = 0; i < size_; ++i)
            tree_[i + 1] = source[i];
        for (int i = 1; i <= size_; ++i) {
            int parent = i + (i & -i);
            if (parent <= size_)
                tree_[parent] = operation_(tree_[parent], tree_[i]);
        }
    }

    int len() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    const O& operation() const noexcept { return operation_; }

    void clear() { fill(tree_.begin(), tree_.end(), operation_.id()); }

    void add(int index, const T& delta) {
        npre(0 <= index && index < size_);
        for (++index; index <= size_; index += index & -index)
            tree_[index] = operation_(tree_[index], delta);
    }

    T prefix(int right) const {
        npre(0 <= right && right <= size_);
        T result = operation_.id();
        for (; right; right -= right & -right)
            result = operation_(tree_[right], result);
        return result;
    }

    T fold(int left, int right) const
        requires ngroup<O, T>
    {
        npre(0 <= left && left <= right && right <= size_);
        return operation_(operation_.inv(prefix(left)), prefix(right));
    }

    T get(int index) const
        requires ngroup<O, T>
    {
        npre(0 <= index && index < size_);
        return fold(index, index + 1);
    }

    template <class C = nless<>> int lower(const T& target, C less = {}) const {
        if (!less(operation_.id(), target))
            return 0;
        int index = 0;
        T prefix_value = operation_.id();
        for (int step = int(bit_floor(unsigned(size_))); step; step >>= 1) {
            int next = index + step;
            if (next <= size_) {
                T candidate = operation_(prefix_value, tree_[next]);
                if (less(candidate, target)) {
                    index = next;
                    prefix_value = move(candidate);
                }
            }
        }
        return index == size_ ? npos : index;
    }
};

template <class T, class O = nadd<T>>
    requires nmonoid<O, T>
class nseg {
    [[no_unique_address]] O operation_;
    int size_ = 0, base_ = 1;
    vector<T> tree_;

    static int checked_size(int n) {
        npre(0 <= n && n <= (1 << 30));
        return n;
    }

  public:
    nseg() : tree_(2, operation_.id()) {}
    explicit nseg(int n, O operation = {})
        : operation_(move(operation)), size_(checked_size(n)), base_(nbitceil(size_)),
          tree_(size_t(2) * base_, operation_.id()) {}

    template <nindexed A>
    explicit nseg(const A& source, O operation = {}) : nseg(nlen(source), move(operation)) {
        for (int i = 0; i < size_; ++i)
            tree_[base_ + i] = source[i];
        for (int i = base_; --i > 0;)
            tree_[i] = operation_(tree_[i << 1], tree_[i << 1 | 1]);
    }

    int len() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    const O& operation() const noexcept { return operation_; }

    void clear() { fill(tree_.begin(), tree_.end(), operation_.id()); }

    void set(int index, T value) {
        npre(0 <= index && index < size_);
        int node = base_ + index;
        tree_[node] = move(value);
        while (node >>= 1)
            tree_[node] = operation_(tree_[node << 1], tree_[node << 1 | 1]);
    }

    T get(int index) const {
        npre(0 <= index && index < size_);
        return tree_[base_ + index];
    }

    T fold(int left, int right) const {
        npre(0 <= left && left <= right && right <= size_);
        T prefix = operation_.id(), suffix = operation_.id();
        for (left += base_, right += base_; left < right; left >>= 1, right >>= 1) {
            if (left & 1)
                prefix = operation_(move(prefix), tree_[left++]);
            if (right & 1)
                suffix = operation_(tree_[--right], move(suffix));
        }
        return operation_(move(prefix), move(suffix));
    }

    T fold() const { return size_ ? tree_[1] : operation_.id(); }
};

template <class T, class O = nadd<T>> using nseg_iter = nseg<T, O>;
