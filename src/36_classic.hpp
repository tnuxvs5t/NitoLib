template <class T, class O = nmin<T>> class nsparse {
    nvector<T> values_;
    nvector<nvector<T>> table_;
    [[no_unique_address]] O operation_{};

  public:
    nsparse() = default;

    template <nindexed A> explicit nsparse(const A& values, O operation = {})
        : values_(ncollect<T>(values)), operation_(move(operation)) {
        int n = values_.len();
        int levels = n <= 1 ? 0 : int(bit_width(unsigned(n - 1)));
        table_.resize(levels);
        for (int level = 0; level < levels; ++level) {
            table_[level] = values_;
            int half = 1 << level;
            int block = half << 1;
            for (int start = 0; start < n; start += block) {
                int middle = min(start + half, n), last = min(start + block, n);
                if (start < middle) {
                    for (int index = middle - 1; index-- > start;)
                        table_[level][index] =
                            operation_(values_[index], table_[level][index + 1]);
                }
                if (middle < last) {
                    for (int index = middle + 1; index < last; ++index)
                        table_[level][index] =
                            operation_(table_[level][index - 1], values_[index]);
                }
            }
        }
    }

    int len() const noexcept { return values_.len(); }
    bool empty() const noexcept { return values_.empty(); }
    const T& operator[](int index) const { return values_[index]; }
    T fold(int left, int right) const {
        npre(0 <= left && left <= right && right <= len());
        if (left == right)
            return operation_.id();
        if (left + 1 == right)
            return values_[left];
        int level = int(bit_width(unsigned(left ^ (right - 1)))) - 1;
        return operation_(table_[level][left], table_[level][right - 1]);
    }
    T fold(int left, int right, T fallback) const {
        npre(0 <= left && left <= right && right <= len());
        return left == right ? move(fallback) : fold(left, right);
    }
};

template <class T>
    requires copyable<T> && requires(T a, const T& b) {
        { a += b } -> same_as<T&>;
        { -a } -> convertible_to<T>;
        { a == b } -> convertible_to<bool>;
    }
class npotential_dsu {
    vector<int> parent_;
    nvector<T> delta_; // potential(vertex) - potential(parent(vertex))

    static size_t checked_size(int n) {
        npre(n >= 0);
        return size_t(n);
    }
    static T add(T left, const T& right) { return left += right; }
    static T sub(T left, const T& right) { return left += -right; }

    pair<int, T> root_with_potential(int vertex) {
        npre(0 <= vertex && vertex < len());
        int root = vertex;
        T total{};
        while (parent_[root] >= 0) {
            total = add(move(total), delta_[root]);
            root = parent_[root];
        }
        int current = vertex;
        T prefix{};
        while (parent_[current] >= 0) {
            int next = parent_[current];
            T edge = delta_[current];
            parent_[current] = root;
            delta_[current] = sub(total, prefix);
            prefix = add(move(prefix), edge);
            current = next;
        }
        return {root, total};
    }

  public:
    explicit npotential_dsu(int n = 0) : parent_(checked_size(n), -1), delta_(n) {}
    int len() const noexcept { return int(parent_.size()); }

    int find(int vertex) { return root_with_potential(vertex).first; }
    int size(int vertex) { return -parent_[find(vertex)]; }
    bool same(int a, int b) { return find(a) == find(b); }

    // Enforces potential(right) - potential(left) == difference.
    bool bind(int left, int right, T difference) {
        auto [left_root, left_value] = root_with_potential(left);
        auto [right_root, right_value] = root_with_potential(right);
        if (left_root == right_root)
            return sub(right_value, left_value) == difference;

        T right_to_left = add(add(move(difference), left_value), -right_value);
        if (parent_[left_root] > parent_[right_root]) {
            swap(left_root, right_root);
            right_to_left = -right_to_left;
        }
        parent_[left_root] += parent_[right_root];
        parent_[right_root] = left_root;
        delta_[right_root] = move(right_to_left);
        return true;
    }
    nmaybe<T> diff(int left, int right) {
        auto [left_root, left_value] = root_with_potential(left);
        auto [right_root, right_value] = root_with_potential(right);
        if (left_root != right_root)
            return {};
        return sub(right_value, left_value);
    }
    T diff(int left, int right, T fallback) {
        auto result = diff(left, right);
        return result ? result.val() : move(fallback);
    }
};
