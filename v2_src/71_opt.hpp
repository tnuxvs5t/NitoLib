template <class T> struct nline_function {
    T slope{}, intercept{};
    using value_type = nwide_t<T>;
    constexpr value_type operator()(T x) const {
        return ni::nchecked_add(ni::nchecked_mul(ni::ngeom_widen(slope), ni::ngeom_widen(x)),
                                ni::ngeom_widen(intercept));
    }
};

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
