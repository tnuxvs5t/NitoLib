template <unsigned_integral T = uint64_t> class nxorbasis {
    static constexpr int bits = numeric_limits<T>::digits;
    array<T, bits> basis_{};
    int rank_ = 0;

  public:
    int len() const noexcept { return rank_; }
    bool empty() const noexcept { return rank_ == 0; }

    bool ins(T value) {
        for (int bit = bits; bit-- > 0;)
            if ((value >> bit) & T{1}) {
                if (basis_[bit])
                    value ^= basis_[bit];
                else {
                    basis_[bit] = value;
                    ++rank_;
                    return true;
                }
            }
        return false;
    }
    bool has(T value) const {
        for (int bit = bits; bit-- > 0;)
            if ((value >> bit) & T{1})
                value ^= basis_[bit];
        return value == T{};
    }
    T max(T initial = T{}) const {
        for (int bit = bits; bit-- > 0;)
            nchmax(initial, T(initial ^ basis_[bit]));
        return initial;
    }
    T min_nonzero(T fallback = T{}) const {
        for (T value : basis_)
            if (value)
                return value;
        return fallback;
    }
};

template <class P = long double> class nprob {
    nvector<P> weights_;

    static bool valid_weight(const P& value) {
        if constexpr (floating_point<P>)
            return isfinite(value) && !(value < P{});
        else
            return !(value < P{});
    }

  public:
    using value_type = P;

    nprob() = default;
    explicit nprob(int count, P value = P{}) : weights_(count, move(value)) {}
    nprob(initializer_list<P> values) : weights_(values) {}

    int len() const noexcept { return weights_.len(); }
    bool empty() const noexcept { return weights_.empty(); }
    P& operator[](int index) { return weights_[index]; }
    const P& operator[](int index) const { return weights_[index]; }
    P get(int index, P fallback = P{}) const { return weights_.get(index, move(fallback)); }

    P sum() const {
        P result{};
        for (int index = 0; index < len(); ++index)
            result += weights_[index];
        return result;
    }
    bool nonnegative() const {
        for (int index = 0; index < len(); ++index)
            if (!valid_weight(weights_[index]))
                return false;
        return true;
    }
    bool is_normalized(P total = P{1}) const {
        if (!nonnegative())
            return false;
        P actual = sum();
        if constexpr (floating_point<P>)
            return isfinite(actual) && actual == total;
        else
            return actual == total;
    }

    nprob& operator*=(P factor) {
        for (int index = 0; index < len(); ++index)
            weights_[index] *= factor;
        return *this;
    }
    friend nprob operator*(nprob distribution, P factor) { return distribution *= factor; }
    friend nprob operator*(P factor, nprob distribution) { return distribution *= factor; }

    template <class F> auto expect(F evaluate) const {
        using R = remove_cvref_t<invoke_result_t<F&, int>>;
        R result{};
        for (int index = 0; index < len(); ++index)
            result += weights_[index] * invoke(evaluate, index);
        return result;
    }
    nmaybe<nprob> normalized_copy(P total = P{1}) const {
        P current = sum();
        if (!nonnegative() || !(current > P{}))
            return {};
        if constexpr (floating_point<P>)
            if (!isfinite(current) || !isfinite(total))
                return {};
        nprob result = *this;
        for (int index = 0; index < result.len(); ++index)
            result[index] = result[index] * total / current;
        return result;
    }
    nmaybe<nprob> normalized(P total = P{1}) const {
        return normalized_copy(total);
    }
    nprob& normalize(P total = P{1}) {
        auto result = normalized_copy(total);
        npre(result.ok());
        *this = move(result.val());
        return *this;
    }

    int draw(nrng& random = nrng_global, int fallback = npos) const
        requires floating_point<P>
    {
        long double total = static_cast<long double>(sum());
        if (!nonnegative() || !(total > 0) || !isfinite(total))
            return fallback;
        long double unit = ldexp(static_cast<long double>(random() >> 11), -53);
        long double remaining = unit * total;
        for (int index = 0; index < len(); ++index) {
            remaining -= static_cast<long double>(weights_[index]);
            if (remaining < 0)
                return index;
        }
        return empty() ? fallback : len() - 1;
    }

    friend bool operator==(const nprob&, const nprob&) = default;
};

template <class P, class F> auto nexpect(const nprob<P>& distribution, F evaluate) {
    return distribution.expect(move(evaluate));
}

template <unsigned_integral T = uint64_t> class nnim {
  public:
    nvector<T> h;

  private:
    T xor_{};

  public:
    nnim() = default;
    template <class A>
        requires nenumerable<const A&>
    explicit nnim(const A& heaps) {
        nfor(value, heaps)
            push(T(value));
    }

    int len() const noexcept { return h.len(); }
    bool empty() const noexcept { return h.empty(); }
    void push(T value) {
        h.push(value);
        xor_ ^= value;
    }
    bool win() const noexcept { return xor_ != T{}; }
    T nim_sum() const noexcept { return xor_; }
    nmaybe<pair<int, T>> winning() const {
        if (!win())
            return {};
        for (int index = 0; index < len(); ++index) {
            T reduced = h[index] ^ xor_;
            if (reduced < h[index])
                return pair<int, T>{index, reduced};
        }
        npre(false);
        return {};
    }
    pair<int, T> winning(pair<int, T> fallback) const {
        auto result = winning();
        return result ? result.val() : move(fallback);
    }
};

template <ngraph_like G> nmaybe<nvector<int>> nsg_dag(const G& graph) {
    auto order = ntoposort(graph);
    if (!order)
        return {};
    int vertices = ni::ngraph_vertices(graph);
    nvector<int> grundy(vertices, 0), mark(vertices + 1, npos);
    for (int position = order->len(); position-- > 0;) {
        int vertex = (*order)[position];
        int stamp = position;
        decltype(auto) adjacency = graph.neighbors(vertex);
        nfor(edge, adjacency) {
            int to = nedge_to(edge);
            npre(0 <= to && to < vertices);
            if (grundy[to] <= vertices)
                mark[grundy[to]] = stamp;
        }
        int value = 0;
        while (value <= vertices && mark[value] == stamp)
            ++value;
        grundy[vertex] = value;
    }
    return grundy;
}

template <ngraph_like G> nvector<int> nsg_dag(const G& graph, nvector<int> fallback) {
    auto result = nsg_dag(graph);
    return result ? move(result.val()) : move(fallback);
}

template <ngraph_like G> nmaybe<nvector<int>> nsg(const G& graph) { return nsg_dag(graph); }

template <ngraph_like G> nvector<int> nsg(const G& graph, nvector<int> fallback) {
    return nsg_dag(graph, move(fallback));
}
