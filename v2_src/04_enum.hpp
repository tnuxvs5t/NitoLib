template <signed_integral T> class nrange_t {
    T first_ = 0, last_ = 0, step_ = 1;

  public:
    constexpr nrange_t() = default;
    constexpr nrange_t(T first, T last, T step = 1) : first_(first), last_(last), step_(step) {
        npre(step != 0);
    }

    constexpr int len() const {
        using W = __int128_t;
        W first = first_, last = last_, step = step_;
        if ((step > 0 && first >= last) || (step < 0 && first <= last))
            return 0;
        W distance = step > 0 ? last - first : first - last;
        W stride = step > 0 ? step : -step;
        W count = (distance + stride - 1) / stride;
        npre(count <= INT_MAX);
        return int(count);
    }
    constexpr bool empty() const { return len() == 0; }

    struct cursor {
        T value, last, step;
        int index = 0;

        constexpr bool ok() const { return step > 0 ? value < last : value > last; }
        constexpr T val() const { return value; }
        constexpr int idx() const { return index; }
        constexpr void next() {
            using W = __int128_t;
            W next = W(value) + step;
            if (next < numeric_limits<T>::lowest() || next > numeric_limits<T>::max())
                value = last;
            else
                value = T(next);
            ++index;
        }
    };

    constexpr cursor enumerate() const { return {first_, last_, step_}; }
};

template <signed_integral T> constexpr auto nrange(T last) { return nrange_t<T>(0, last); }
template <signed_integral T> constexpr auto nrange(T first, T last) { return nrange_t<T>(first, last); }
template <signed_integral T> constexpr auto nrange(T first, T last, T step) {
    return nrange_t<T>(first, last, step);
}

template <integral I> constexpr int ni_nloop_count(I count) {
    if constexpr (signed_integral<I>) {
        if (count <= 0)
            return 0;
    } else if (count == 0) {
        return 0;
    }
    npre(uint64_t(count) <= uint64_t(INT_MAX));
    return int(count);
}

namespace ni {
template <class A> struct nborrowed_index_cursor {
    A* owner;
    int index = 0;
    bool ok() const { return index < nlen(*owner); }
    decltype(auto) val() const { return (*owner)[index]; }
    int idx() const { return index; }
    void next() { ++index; }
};

template <class A> struct nowned_index_cursor {
    A owner;
    int index = 0;
    bool ok() const { return index < nlen(owner); }
    decltype(auto) val() { return owner[index]; }
    int idx() const { return index; }
    void next() { ++index; }
};
} // namespace ni

template <class A>
    requires requires(A&& a) { forward<A>(a).enumerate(); }
constexpr decltype(auto) nenumerate(A&& a) {
    return forward<A>(a).enumerate();
}

template <class A>
    requires nindexed<A> && (!requires(A& a) { a.enumerate(); })
constexpr auto nenumerate(A& a) {
    return ni::nborrowed_index_cursor<A>{addressof(a)};
}

template <class A>
    requires(!is_lvalue_reference_v<A> && nindexed<remove_reference_t<A>> &&
             (!requires(remove_reference_t<A>& a) { a.enumerate(); }))
constexpr auto nenumerate(A&& a) {
    return ni::nowned_index_cursor<remove_cvref_t<A>>{forward<A>(a)};
}

namespace ni {
struct ncursor_sentinel {};

template <class Reference> struct ncursor_entry {
    int index;
    Reference value;
};

template <class Cursor, bool WithIndex> class ncursor_iterator {
    Cursor* cursor_;

  public:
    constexpr explicit ncursor_iterator(Cursor* cursor) : cursor_(cursor) {}
    constexpr bool operator!=(ncursor_sentinel) const { return cursor_->ok(); }
    constexpr decltype(auto) operator*() const {
        if constexpr (WithIndex) {
            using Reference = decltype(cursor_->val());
            return ncursor_entry<Reference>{cursor_->idx(), cursor_->val()};
        } else {
            return cursor_->val();
        }
    }
    constexpr ncursor_iterator& operator++() {
        cursor_->next();
        return *this;
    }
};

template <class Cursor, bool WithIndex> struct ncursor_range {
    Cursor cursor;
    constexpr auto begin() { return ncursor_iterator<Cursor, WithIndex>{addressof(cursor)}; }
    constexpr auto end() const { return ncursor_sentinel{}; }
};
} // namespace ni

template <bool WithIndex, class A> constexpr auto ni_ncursor_range(A&& a) {
    using Cursor = remove_cvref_t<decltype(nenumerate(forward<A>(a)))>;
    return ni::ncursor_range<Cursor, WithIndex>{nenumerate(forward<A>(a))};
}

#define nfor(x, sequence) for (auto&& x : ni_ncursor_range<false>((sequence)))
#define nfori(index, x, sequence) for (auto&& [index, x] : ni_ncursor_range<true>((sequence)))
#define nrep(index, count) for (int index : ni_ncursor_range<false>(nrange(ni_nloop_count((count)))))
#define nrrep(index, count)                                                                                           \
    for (int index : ni_ncursor_range<false>(nrange(ni_nloop_count((count)) - 1, -1, -1)))

template <class A> constexpr auto nreverse(A& a) {
    return nstride(a, nlen(a) - 1, nlen(a), -1);
}

template <class A, class F> constexpr auto nproject(A& a, F projection) {
    return nview(nlen(a), [owner = addressof(a), projection = move(projection)](int i) -> decltype(auto) {
        return invoke(projection, (*owner)[i]);
    });
}

template <class A, class B> class nzip_view {
    A* left_;
    B* right_;

  public:
    constexpr nzip_view(A& left, B& right) : left_(addressof(left)), right_(addressof(right)) {}
    constexpr int len() const { return min(nlen(*left_), nlen(*right_)); }
    constexpr bool empty() const { return len() == 0; }
    constexpr auto operator[](int i) const {
        npre(0 <= i && i < len());
        using L = decltype((*left_)[i]);
        using R = decltype((*right_)[i]);
        return pair<L, R>((*left_)[i], (*right_)[i]);
    }
};

template <class A, class B> constexpr auto nzip(A& left, B& right) { return nzip_view<A, B>(left, right); }

template <class A, class B> class nproduct_view {
    A* left_;
    B* right_;
    int left_size_, right_size_;

  public:
    constexpr nproduct_view(A& left, B& right)
        : left_(addressof(left)), right_(addressof(right)), left_size_(nlen(left)), right_size_(nlen(right)) {
        npre(nlen(left) == 0 || nlen(right) <= INT_MAX / nlen(left));
    }
    constexpr int len() const { return left_size_ * right_size_; }
    constexpr bool empty() const { return len() == 0; }
    constexpr auto operator[](int i) const {
        npre(0 <= i && i < len());
        int width = right_size_;
        using L = decltype((*left_)[i / width]);
        using R = decltype((*right_)[i % width]);
        return pair<L, R>((*left_)[i / width], (*right_)[i % width]);
    }
};

template <class A, class B> constexpr auto nproduct(A& left, B& right) {
    return nproduct_view<A, B>(left, right);
}

template <class A> class nwindow_view {
    A* owner_;
    int width_, step_, size_;

    static constexpr int count(const A& owner, int width, int step) {
        npre(width >= 0 && step > 0);
        return width <= nlen(owner) ? 1 + (nlen(owner) - width) / step : 0;
    }

  public:
    constexpr nwindow_view(A& owner, int width, int step)
        : owner_(addressof(owner)), width_(width), step_(step),
          size_(count(owner, width, step)) {}
    constexpr int len() const { return size_; }
    constexpr bool empty() const { return size_ == 0; }
    constexpr auto operator[](int i) const {
        npre(0 <= i && i < size_);
        int first = i * step_;
        return nsub(*owner_, first, first + width_);
    }
};

template <class A> constexpr auto nwindows(A& owner, int width, int step = 1) {
    return nwindow_view<A>(owner, width, step);
}
