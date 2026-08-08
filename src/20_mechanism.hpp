/**
 * Binary exponentiation for an associative operation with id().  Negative exponents
 * are accepted only when O has a usable inv(value); otherwise the checked profile
 * rejects the call.  The operation must not change meaning between steps.
 */
template <class T, class O = nmul<T>>
constexpr T npow(T base, long long exponent, O operation = {}) {
    uint64_t remaining;
    if (exponent < 0) {
        if constexpr (requires { operation.inv(move(base)); }) {
            base = operation.inv(move(base));
            remaining = uint64_t{} - uint64_t(exponent);
        } else {
            npre(exponent >= 0);
            return operation.id();
        }
    } else {
        remaining = uint64_t(exponent);
    }
    T result = operation.id();
    while (remaining) {
        if (remaining & 1)
            result = operation(move(result), base);
        remaining >>= 1;
        if (remaining) {
            T copy = base;
            base = operation(move(copy), base);
        }
    }
    return result;
}

// Prefix/suffix scans require an identity and associative ordered combine.  Non-
// commutative operations are supported because each direction is explicit.
template <class A, class O = nadd<nindex_value_t<const A>>>
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

// The predicate must be false-then-true on [first,last); otherwise binary search has
// no correctness guarantee.  nlast_true uses the dual true-then-false convention.
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

/**
 * Address-based rollback log.  Every saved target must outlive the log until rollback;
 * moving/reallocating the target behind the log is a dangling-pointer bug.  Checkpoints
 * are stack times and rollback restores in strict reverse order.
 */
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
