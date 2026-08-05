template <class A, class O = nadd<nindex_value_t<const A>>>
    requires nmonoid<O, nindex_value_t<const A>>
auto nscan(const A& a, O op = {}) {
    using T = nindex_value_t<const A>;
    npre(nlen(a) < INT_MAX);
    nvector<T> result;
    result.reserve(nlen(a) + 1);
    result.push(op.id());
    for (int i = 0; i < nlen(a); ++i)
        result.push(op(result.back(), a[i]));
    return result;
}

template <class A, class O = nadd<nindex_value_t<const A>>>
    requires nmonoid<O, nindex_value_t<const A>>
auto nsuffix_scan(const A& a, O op = {}) {
    using T = nindex_value_t<const A>;
    npre(nlen(a) < INT_MAX);
    nvector<T> reversed;
    reversed.reserve(nlen(a) + 1);
    reversed.push(op.id());
    for (int i = nlen(a); i-- > 0;)
        reversed.push(op(a[i], reversed.back()));
    nreverse_inplace(reversed);
    return reversed;
}

template <signed_integral I, class P> constexpr I nfirst_true(I first, I last, P predicate) {
    npre(first <= last);
    while (first < last) {
        I middle = midpoint(first, last);
        predicate(middle) ? last = middle : first = middle + 1;
    }
    return first;
}

template <signed_integral I, class P> constexpr I nlast_true(I first, I last, P predicate) {
    npre(first <= last && first > numeric_limits<I>::lowest());
    while (first < last) {
        I middle = midpoint(first, last);
        if (predicate(middle))
            first = middle + 1;
        else
            last = middle;
    }
    return first - 1;
}

template <class T> class nrollback {
    struct change {
        T* target;
        T old_value;
    };
    vector<change> history_;

  public:
    int time() const noexcept {
        npre(history_.size() <= size_t(INT_MAX));
        return int(history_.size());
    }
    bool empty() const noexcept { return history_.empty(); }
    void reserve(int n) {
        npre(n >= 0);
        history_.reserve(size_t(n));
    }
    void save(T& target) {
        npre(history_.size() < size_t(INT_MAX));
        history_.push_back({addressof(target), target});
    }
    void assign(T& target, T value) {
        npre(history_.size() < size_t(INT_MAX));
        history_.push_back({addressof(target), move(target)});
        target = move(value);
    }
    template <class F> decltype(auto) mutate(T& target, F&& mutation) {
        save(target);
        return invoke(forward<F>(mutation), target);
    }
    void undo() {
        npre(!history_.empty());
        auto change = move(history_.back());
        history_.pop_back();
        *change.target = move(change.old_value);
    }
    void rollback(int checkpoint) {
        npre(0 <= checkpoint && checkpoint <= time());
        while (time() > checkpoint)
            undo();
    }
    void clear() noexcept { history_.clear(); }
};
