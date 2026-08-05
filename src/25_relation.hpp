template <class L, class R, class EL = nequal<>, class ER = nequal<>> class nrel_scan {
    struct edge {
        L left;
        R right;
    };

    vector<edge> edges_;
    [[no_unique_address]] EL equal_left_{};
    [[no_unique_address]] ER equal_right_{};

  public:
    nrel_scan() = default;
    explicit nrel_scan(EL equal_left, ER equal_right = {})
        : equal_left_(move(equal_left)), equal_right_(move(equal_right)) {}

    int len() const {
        npre(edges_.size() <= size_t(INT_MAX));
        return int(edges_.size());
    }
    bool empty() const noexcept { return edges_.empty(); }
    void reserve(int capacity) {
        npre(capacity >= 0);
        edges_.reserve(size_t(capacity));
    }
    void clear() noexcept { edges_.clear(); }
    bool equal_left(const L& a, const L& b) const { return invoke(equal_left_, a, b); }
    bool equal_right(const R& a, const R& b) const { return invoke(equal_right_, a, b); }
    bool has(const L& left, const R& right) const {
        for (const edge& candidate : edges_)
            if (equal_left(candidate.left, left) && equal_right(candidate.right, right))
                return true;
        return false;
    }
    bool add(const L& left, const R& right) {
        if (has(left, right))
            return false;
        npre(edges_.size() < size_t(INT_MAX));
        edges_.push_back({left, right});
        return true;
    }
    bool add(L&& left, R&& right) {
        if (has(left, right))
            return false;
        npre(edges_.size() < size_t(INT_MAX));
        edges_.push_back({move(left), move(right)});
        return true;
    }
    bool del(const L& left, const R& right) {
        for (int index = 0; index < len(); ++index)
            if (equal_left(edges_[index].left, left) && equal_right(edges_[index].right, right)) {
                if (index + 1 != len())
                    edges_[index] = move(edges_.back());
                edges_.pop_back();
                return true;
            }
        return false;
    }
    nvector<R> image(const L& left) const {
        nvector<R> result;
        for (const edge& candidate : edges_)
            if (equal_left(candidate.left, left))
                result.push(candidate.right);
        return result;
    }
    nvector<L> preimage(const R& right) const {
        nvector<L> result;
        for (const edge& candidate : edges_)
            if (equal_right(candidate.right, right))
                result.push(candidate.left);
        return result;
    }

    struct cursor {
        const nrel_scan* owner;
        int index = 0;
        bool ok() const { return index < owner->len(); }
        auto val() const {
            return pair<const L&, const R&>(owner->edges_[index].left,
                                            owner->edges_[index].right);
        }
        int idx() const { return index; }
        void next() { ++index; }
    };
    cursor enumerate() const& { return {this}; }
    cursor enumerate() && = delete;

    nrel_scan& operator|=(const nrel_scan& other) {
        if (this != addressof(other))
            nfor(edge, other)
                add(edge.first, edge.second);
        return *this;
    }
    nrel_scan& operator&=(const nrel_scan& other) {
        if (this == addressof(other))
            return *this;
        vector<edge> kept;
        kept.reserve(edges_.size());
        for (const edge& candidate : edges_)
            if (other.has(candidate.left, candidate.right))
                kept.push_back(candidate);
        edges_ = move(kept);
        return *this;
    }
    nrel_scan& operator-=(const nrel_scan& other) {
        if (this == addressof(other)) {
            clear();
            return *this;
        }
        nfor(edge, other)
            del(edge.first, edge.second);
        return *this;
    }
    nrel_scan& operator^=(const nrel_scan& other) {
        if (this == addressof(other)) {
            clear();
            return *this;
        }
        nfor(edge, other) {
            if (has(edge.first, edge.second))
                del(edge.first, edge.second);
            else
                add(edge.first, edge.second);
        }
        return *this;
    }
    friend nrel_scan operator|(nrel_scan left, const nrel_scan& right) { return left |= right; }
    friend nrel_scan operator&(nrel_scan left, const nrel_scan& right) { return left &= right; }
    friend nrel_scan operator-(nrel_scan left, const nrel_scan& right) { return left -= right; }
    friend nrel_scan operator^(nrel_scan left, const nrel_scan& right) { return left ^= right; }
    friend bool operator==(const nrel_scan& left, const nrel_scan& right) {
        if (left.len() != right.len())
            return false;
        for (const edge& candidate : left.edges_)
            if (!right.has(candidate.left, candidate.right))
                return false;
        return true;
    }
};

template <class L, class R, class EL = nequal<>, class ER = nequal<>>
using nrel = nrel_scan<L, R, EL, ER>;

template <class A, class B, class HA = nhash<A>, class HB = nhash<B>,
          class EA = equal_to<A>, class EB = equal_to<B>>
class npartial_hash {
    nmap_flat<A, B, HA, EA> forward_;
    [[no_unique_address]] EB equal_value_{};

  public:
    npartial_hash() = default;
    explicit npartial_hash(EB equal_value) : equal_value_(move(equal_value)) {}

    int len() const { return forward_.len(); }
    bool empty() const { return forward_.empty(); }
    void reserve(int capacity) { forward_.reserve(capacity); }
    void clear() { forward_.clear(); }
    bool has(const A& argument) const { return forward_.has(argument); }
    B* to(const A& argument) { return forward_.get(argument); }
    const B* to(const A& argument) const { return forward_.get(argument); }
    B to(const A& argument, B fallback) const {
        return forward_.get(argument, move(fallback));
    }
    bool bind(const A& argument, const B& value) {
        B* current = forward_.get(argument);
        return current ? invoke(equal_value_, *current, value) : forward_.ins(argument, value);
    }
    template <class X> B& set(const A& argument, X&& value) {
        return forward_.set(argument, forward<X>(value));
    }
    bool unbind(const A& argument) { return forward_.del(argument) != 0; }
    B& operator()(const A& argument) { return forward_(argument); }
    const B& operator()(const A& argument) const { return forward_(argument); }
    auto enumerate() & { return forward_.enumerate(); }
    auto enumerate() const& { return forward_.enumerate(); }
    auto enumerate() && = delete;
    friend bool operator==(const npartial_hash& left, const npartial_hash& right) {
        return left.forward_ == right.forward_;
    }
};

template <class A, class B, class HA = nhash<A>, class HB = nhash<B>,
          class EA = equal_to<A>, class EB = equal_to<B>>
using nfunc_hash = npartial_hash<A, B, HA, HB, EA, EB>;

template <class A, class B, class HA = nhash<A>, class HB = nhash<B>,
          class EA = equal_to<A>, class EB = equal_to<B>>
using npartial = npartial_hash<A, B, HA, HB, EA, EB>;

template <class A, class B, class HA = nhash<A>, class HB = nhash<B>,
          class EA = equal_to<A>, class EB = equal_to<B>>
class nbije_hash {
    template <class, class, class, class, class, class> friend class nbije_hash;

    nmap_flat<A, B, HA, EA> forward_;
    nmap_flat<B, A, HB, EB> backward_;
    [[no_unique_address]] EA equal_left_{};
    [[no_unique_address]] EB equal_right_{};

  public:
    int len() const { return forward_.len(); }
    bool empty() const { return forward_.empty(); }
    void reserve(int capacity) {
        forward_.reserve(capacity);
        backward_.reserve(capacity);
    }
    void clear() {
        forward_.clear();
        backward_.clear();
    }
    bool hasl(const A& left) const { return forward_.has(left); }
    bool hasr(const B& right) const { return backward_.has(right); }
    const B* to(const A& left) const { return forward_.get(left); }
    const A* from(const B& right) const { return backward_.get(right); }
    B to(const A& left, B fallback) const { return forward_.get(left, move(fallback)); }
    A from(const B& right, A fallback) const { return backward_.get(right, move(fallback)); }
    const B& operator()(const A& left) const { return forward_(left); }

    bool bind(const A& left, const B& right) {
        const B* existing_right = to(left);
        const A* existing_left = from(right);
        if (existing_right || existing_left)
            return existing_right && existing_left &&
                   invoke(equal_right_, *existing_right, right) &&
                   invoke(equal_left_, *existing_left, left);
        bool inserted_forward = forward_.ins(left, right);
        bool inserted_backward = backward_.ins(right, left);
        npre(inserted_forward && inserted_backward);
        return true;
    }
    bool unbindl(const A& left) {
        const B* right = to(left);
        if (!right)
            return false;
        B copy = *right;
        npre(forward_.del(left) == 1);
        npre(backward_.del(copy) == 1);
        return true;
    }
    bool unbindr(const B& right) {
        const A* left = from(right);
        if (!left)
            return false;
        A copy = *left;
        npre(backward_.del(right) == 1);
        npre(forward_.del(copy) == 1);
        return true;
    }
    void set(const A& left, const B& right) {
        unbindl(left);
        unbindr(right);
        npre(bind(left, right));
    }
    auto inverse() const {
        nbije_hash<B, A, HB, HA, EB, EA> result;
        result.forward_ = backward_;
        result.backward_ = forward_;
        result.equal_left_ = equal_right_;
        result.equal_right_ = equal_left_;
        return result;
    }
    auto operator~() const { return inverse(); }
    auto enumerate() const& { return forward_.enumerate(); }
    auto enumerate() && = delete;

    template <class X, class HX, class EX>
    auto operator*(const nbije_hash<X, A, HX, HA, EX, EA>& inner) const {
        nbije_hash<X, B, HX, HB, EX, EB> result;
        nforkv(argument, middle, inner)
            if (const B* value = to(middle))
                result.bind(argument, *value);
        return result;
    }
    friend bool operator==(const nbije_hash& left, const nbije_hash& right) {
        return left.forward_ == right.forward_;
    }
};

template <class T, class C = nless<T>> class nbije_rank {
    nvector<T> values_;
    [[no_unique_address]] C compare_{};

    bool equivalent(const T& a, const T& b) const {
        return !invoke(compare_, a, b) && !invoke(compare_, b, a);
    }

  public:
    nbije_rank() = default;

    template <class A>
        requires nenumerable<const A&>
    explicit nbije_rank(const A& source, C compare = {})
        : values_(ncollect<T>(source)), compare_(move(compare)) {
        nsort(values_, compare_);
        nunique(values_, [this](const T& a, const T& b) { return equivalent(a, b); });
    }

    int len() const noexcept { return values_.len(); }
    bool empty() const noexcept { return values_.empty(); }
    int to(const T& value) const {
        int index = nlower(values_, value, compare_);
        return index < len() && equivalent(value, values_[index]) ? index : npos;
    }
    int to(const T& value, int fallback) const {
        int index = to(value);
        return index == npos ? fallback : index;
    }
    bool hasl(const T& value) const { return to(value) != npos; }
    bool hasr(int index) const { return 0 <= index && index < len(); }
    const T* from(int index) const { return values_.get(index); }
    T from(int index, T fallback) const { return values_.get(index, move(fallback)); }
    const T& operator()(int index) const {
        npre(hasr(index));
        return values_[index];
    }

    struct cursor {
        const nbije_rank* owner;
        int index = 0;
        bool ok() const { return index < owner->len(); }
        const T& key() const { return owner->values_[index]; }
        int val() const { return index; }
        int idx() const { return index; }
        void next() { ++index; }
    };
    cursor enumerate() const& { return {this}; }
    cursor enumerate() && = delete;

    auto inverse() const {
        nbije_hash<int, T> result;
        result.reserve(len());
        for (int index = 0; index < len(); ++index)
            result.bind(index, values_[index]);
        return result;
    }
    auto operator~() const { return inverse(); }
};

template <class A, class C = nless<nindex_value_t<const A>>>
    requires nenumerable<const A&>
auto ncompress(const A& source, C compare = {}) {
    using T = nindex_value_t<const A>;
    return nbije_rank<T, C>(source, move(compare));
}

template <class A, class C = nless<typename A::value_type>>
auto ncompress_stl(const A& source, C compare = {}) {
    using T = typename A::value_type;
    nvector<T> values;
    if constexpr (requires { source.size(); })
        values.reserve(nlen(source));
    for (const T& value : source)
        values.push(value);
    return nbije_rank<T, C>(values, move(compare));
}

template <class A, class B, class HA = nhash<A>, class HB = nhash<B>,
          class EA = equal_to<A>, class EB = equal_to<B>>
using nbije = nbije_hash<A, B, HA, HB, EA, EB>;

template <class A, class B, class HA = nhash<A>, class HB = nhash<B>,
          class EA = equal_to<A>, class EB = equal_to<B>>
using ninj = nbije_hash<A, B, HA, HB, EA, EB>;
