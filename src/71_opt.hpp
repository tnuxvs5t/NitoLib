// Affine function evaluated in nwide_t<T>; callers still ensure slope*x+intercept fits
// that widened representation and choose Better consistently for min/max queries.
template <class T> struct nline_function {
    T slope{}, intercept{};
    using value_type = nwide_t<T>;
    constexpr value_type operator()(T x) const {
        return ni::nchecked_add(ni::nchecked_mul(ni::ngeom_widen(slope), ni::ngeom_widen(x)),
                                ni::ngeom_widen(intercept));
    }
};

/**
 * Dynamic Li Chao tree on a fixed integer domain.  Inserted functions must be lines so
 * any pair crosses at most once; Better is a strict ordering.  Each add/query is
 * O(log domain_width), and domain endpoints never change after construction.
 */
template <signed_integral T, class Better = nless<nwide_t<T>>> class nlichao {
  public:
    using line_type = nline_function<T>;
    using value_type = typename line_type::value_type;

  private:
    struct node {
        line_type line{};
        int left = npos, right = npos;
        bool used = false;
    };
    T first_{}, last_{}; // Public domain is [first_, last_); recursion uses [first_, last_-1].
    [[no_unique_address]] Better better_;
    nvector<node> nodes_{node{}};

    int child(int vertex, bool right) {
        int next = right ? nodes_[vertex].right : nodes_[vertex].left;
        if (next == npos) {
            next = nodes_.len();
            nodes_.push();
            if (right)
                nodes_[vertex].right = next;
            else
                nodes_[vertex].left = next;
        }
        return next;
    }

    void insert(int vertex, T left, T right, line_type line) {
        if (!nodes_[vertex].used) {
            nodes_[vertex].line = line;
            nodes_[vertex].used = true;
            return;
        }
        T middle = midpoint(left, right);
        bool left_better = invoke(better_, line(left), nodes_[vertex].line(left));
        bool middle_better = invoke(better_, line(middle), nodes_[vertex].line(middle));
        if (middle_better)
            swap(nodes_[vertex].line, line);
        if (left == right)
            return;
        if (left_better != middle_better)
            insert(child(vertex, false), left, middle, line);
        else
            insert(child(vertex, true), T(middle + 1), right, line);
    }

    void insert_segment(int vertex, T left, T right, T query_left, T query_right, line_type line) {
        if (query_left <= left && right <= query_right) {
            insert(vertex, left, right, line);
            return;
        }
        T middle = midpoint(left, right);
        if (query_left <= middle)
            insert_segment(child(vertex, false), left, middle, query_left, query_right, line);
        if (middle < query_right)
            insert_segment(child(vertex, true), T(middle + 1), right, query_left, query_right, line);
    }

    nmaybe<value_type> query(int vertex, T left, T right, T x) const {
        nmaybe<value_type> result;
        if (nodes_[vertex].used)
            result = nodes_[vertex].line(x);
        if (left == right)
            return result;
        T middle = midpoint(left, right);
        int next = x <= middle ? nodes_[vertex].left : nodes_[vertex].right;
        if (next == npos)
            return result;
        auto candidate = x <= middle ? query(next, left, middle, x)
                                     : query(next, T(middle + 1), right, x);
        if (candidate && (!result || invoke(better_, candidate.val(), result.val())))
            result = candidate.val();
        return result;
    }

  public:
    // Integer domain [first,last), matching the rest of Nitori's interval protocol.
    explicit nlichao(T first, T last, Better better = {})
        : first_(first), last_(last), better_(move(better)) {
        npre(first < last);
    }

    int nodes() const noexcept { return nodes_.len(); }
    void add(line_type line) { insert(0, first_, T(last_ - 1), line); }
    void add(T slope, T intercept) { add({slope, intercept}); }
    void add_segment(line_type line, T left, T right) {
        npre(first_ <= left && left <= right && right <= last_);
        if (left < right)
            insert_segment(0, first_, T(last_ - 1), left, T(right - 1), line);
    }
    void add_segment(T slope, T intercept, T left, T right) {
        add_segment({slope, intercept}, left, right);
    }
    nmaybe<value_type> query(T x) const {
        npre(first_ <= x && x < last_);
        return query(0, first_, T(last_ - 1), x);
    }
};

// Compatibility affine line with the same overflow and single-crossing contract as
// nline_function when inserted into a Li Chao structure.
template <class T> struct nline {
    T m{}, b{};
    using value_type = nwide_t<T>;
    constexpr value_type operator()(T x) const {
        return ni::nchecked_add(ni::nchecked_mul(ni::ngeom_widen(m), ni::ngeom_widen(x)),
                                ni::ngeom_widen(b));
    }
    constexpr operator nline_function<T>() const { return {m, b}; }
    friend bool operator==(const nline&, const nline&) = default;
};

// Static-coordinate Li Chao tree has the same single-crossing contract; every queried
// x must belong to the sorted coordinate set supplied at construction.
template <class T, class Better = nless<nwide_t<T>>>
    requires is_arithmetic_v<T> && (!same_as<remove_cv_t<T>, bool>)
class nlichao_static {
  public:
    using line_type = nline<T>;
    using value_type = typename line_type::value_type;
    nvector<T> x;

  private:
    struct node {
        line_type line{};
        bool used = false;
    };
    nvector<node> tree_;
    [[no_unique_address]] Better better_;

    void put(int vertex, int left, int right, line_type line) {
        if (!tree_[vertex].used) {
            tree_[vertex] = node{line, true};
            return;
        }
        int middle = left + (right - left) / 2;
        bool left_better = invoke(better_, line(x[left]), tree_[vertex].line(x[left]));
        bool middle_better = invoke(better_, line(x[middle]), tree_[vertex].line(x[middle]));
        if (middle_better)
            swap(line, tree_[vertex].line);
        if (right - left == 1)
            return;
        if (left_better != middle_better)
            put(vertex * 2, left, middle, line);
        else
            put(vertex * 2 + 1, middle, right, line);
    }
    void put_segment(int vertex, int left, int right, int query_left, int query_right,
                     line_type line) {
        if (query_left <= left && right <= query_right) {
            put(vertex, left, right, line);
            return;
        }
        int middle = left + (right - left) / 2;
        if (query_left < middle)
            put_segment(vertex * 2, left, middle, query_left, query_right, line);
        if (middle < query_right)
            put_segment(vertex * 2 + 1, middle, right, query_left, query_right, line);
    }

  public:
    nlichao_static() : tree_(1) {}

    template <class A>
        requires nenumerable<const A&>
    explicit nlichao_static(const A& coordinates, Better better = {})
        : x(ncollect<T>(coordinates)), better_(move(better)) {
        nsort(x);
        nunique(x);
        npre(x.len() <= (INT_MAX - 1) / 4);
        tree_.resize(max(1, 4 * x.len() + 1));
    }
    int len() const noexcept { return x.len(); }
    bool empty() const noexcept { return x.empty(); }
    bool hasx(const T& value) const {
        int index = nlower(x, value);
        return index < len() && !(value < x[index]) && !(x[index] < value);
    }
    void add(line_type line) {
        if (!empty())
            put(1, 0, len(), line);
    }
    void add(T slope, T intercept) { add(line_type{slope, intercept}); }
    void addidx(line_type line, int left, int right) {
        npre(0 <= left && left <= right && right <= len());
        if (left < right)
            put_segment(1, 0, len(), left, right, line);
    }
    void addseg(line_type line, T left, T right) {
        addidx(line, nlower(x, left), nlower(x, right));
    }
    nmaybe<value_type> get(T value) const {
        int index = nlower(x, value);
        if (index == len() || value < x[index] || x[index] < value)
            return {};
        nmaybe<value_type> result;
        for (int vertex = 1, left = 0, right = len();;) {
            if (tree_[vertex].used) {
                value_type candidate = tree_[vertex].line(value);
                if (!result || invoke(better_, candidate, result.val()))
                    result = candidate;
            }
            if (right - left == 1)
                break;
            int middle = left + (right - left) / 2;
            if (index < middle) {
                right = middle;
                vertex *= 2;
            } else {
                left = middle;
                vertex = vertex * 2 + 1;
            }
        }
        return result;
    }
    value_type get(T value, value_type fallback) const {
        auto result = get(value);
        return result ? result.val() : move(fallback);
    }
    value_type operator()(T value, value_type fallback) const {
        return get(value, move(fallback));
    }
};

// Discrete ternary/unimodal search requires one optimum with monotone improvement then
// worsening over the integer interval; arbitrary functions invalidate the elimination.
template <signed_integral I, class F, class Better = nless<>>
I nunimodal_arg(I first, I last, F function, Better better = {}) {
    npre(first < last);
    auto distance = [](I left, I right) { return __uint128_t(right) - __uint128_t(left); };
    while (distance(first, last) > 4) {
        I third = I(distance(first, last) / 3);
        I left = I(__int128_t(first) + third);
        I right = I(__int128_t(last) - 1 - third);
        auto left_value = invoke(function, left);
        auto right_value = invoke(function, right);
        if (invoke(better, right_value, left_value))
            first = left + 1;
        else
            last = right + 1;
    }
    I best = first;
    auto value = invoke(function, best);
    for (I candidate = first + 1; candidate < last; ++candidate) {
        auto candidate_value = invoke(function, candidate);
        if (invoke(better, candidate_value, value)) {
            best = candidate;
            value = move(candidate_value);
        }
    }
    return best;
}

template <floating_point T, class F>
T nternary_min(T left, T right, F function, int iterations = 100) {
    npre(left <= right && iterations >= 0);
    for (int iteration = 0; iteration < iterations; ++iteration) {
        T distance = (right - left) / T{3};
        T a = left + distance, b = right - distance;
        if (invoke(function, a) < invoke(function, b))
            right = b;
        else
            left = a;
    }
    return midpoint(left, right);
}
