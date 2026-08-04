template <signed_integral T> class nrange_t {
    T first_ = 0, last_ = 0, step_ = 1;

  public:
    using value_type = T;
    using nview_tag = void;

    constexpr nrange_t() = default;
    constexpr nrange_t(T first, T last, T step = 1) : first_(first), last_(last), step_(step) {
        npre(step != 0);
        (void)len();
    }

    constexpr int len() const {
        if ((step_ > 0 && first_ >= last_) || (step_ < 0 && first_ <= last_))
            return 0;
        __uint128_t first = __uint128_t(first_), last = __uint128_t(last_), step = __uint128_t(step_);
        __uint128_t distance = step_ > 0 ? last - first : first - last;
        __uint128_t stride = step_ > 0 ? step : __uint128_t{} - step;
        __uint128_t count = distance / stride + (distance % stride != 0);
        npre(count <= INT_MAX);
        return int(count);
    }
    constexpr bool empty() const { return len() == 0; }
    constexpr T operator[](int index) const {
        npre(0 <= index && index < len());
        return T(__int128_t(first_) + __int128_t(index) * __int128_t(step_));
    }

    struct cursor {
        T value, last, step;
        int index = 0;

        constexpr bool ok() const { return step > 0 ? value < last : value > last; }
        constexpr T val() const { return value; }
        constexpr int idx() const { return index; }
        constexpr void next() {
            bool overflow = step > 0 ? value > numeric_limits<T>::max() - step
                                     : value < numeric_limits<T>::lowest() - step;
            if (overflow)
                value = last;
            else
                value += step;
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
    npre(__uint128_t(count) <= __uint128_t(INT_MAX));
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

template <class A> using nenumerator_t = remove_cvref_t<decltype(nenumerate(declval<A>()))>;

template <class A>
concept nenumerable = requires(A&& sequence) { nenumerate(forward<A>(sequence)); } &&
                      requires(nenumerator_t<A>& cursor) {
                          { cursor.ok() } -> convertible_to<bool>;
                          cursor.val();
                          { cursor.idx() } -> convertible_to<int>;
                          cursor.next();
                      };

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

namespace ni {
template <class Cursor> class nkeyvalue_iterator {
    Cursor* cursor_;

  public:
    constexpr explicit nkeyvalue_iterator(Cursor* cursor) : cursor_(cursor) {}
    constexpr bool operator!=(ncursor_sentinel) const { return cursor_->ok(); }
    constexpr auto operator*() const {
        using key_reference = decltype(cursor_->key());
        using value_reference = decltype(cursor_->val());
        return pair<key_reference, value_reference>(cursor_->key(), cursor_->val());
    }
    constexpr nkeyvalue_iterator& operator++() {
        cursor_->next();
        return *this;
    }
};

template <class Cursor> struct nkeyvalue_range {
    Cursor cursor;
    constexpr auto begin() { return nkeyvalue_iterator<Cursor>{addressof(cursor)}; }
    constexpr auto end() const { return ncursor_sentinel{}; }
};
} // namespace ni

template <class A>
concept nkeyvalue_enumerable = requires(A&& sequence) {
    nenumerate(forward<A>(sequence)).key();
    nenumerate(forward<A>(sequence)).val();
};

template <class A>
    requires nkeyvalue_enumerable<A&&>
constexpr auto ni_nkeyvalue_range(A&& a) {
    using Cursor = remove_cvref_t<decltype(nenumerate(forward<A>(a)))>;
    return ni::nkeyvalue_range<Cursor>{nenumerate(forward<A>(a))};
}

namespace ni {
template <class H> struct nkeyed_index_cursor {
    H owner;
    int index = 0;
    bool ok() const { return index < nlen(owner.get()); }
    decltype(auto) key() { return owner.get().key(index); }
    decltype(auto) val() { return owner.get()[index]; }
    int idx() const { return index; }
    void next() { ++index; }
};
} // namespace ni

template <class A>
    requires(!nkeyvalue_enumerable<A&&>) && nviewable_indexed<A&&> &&
            requires(remove_reference_t<A>& sequence) { sequence.key(0); }
constexpr auto ni_nkeyvalue_range(A&& a) {
    auto owner = ni::nhold_indexed(forward<A>(a));
    using Cursor = ni::nkeyed_index_cursor<decltype(owner)>;
    return ni::nkeyvalue_range<Cursor>{Cursor{move(owner)}};
}

#define nfor(x, sequence) for (auto&& x : ni_ncursor_range<false>((sequence)))
#define nfori(index, x, sequence) for (auto&& [index, x] : ni_ncursor_range<true>((sequence)))
#define nforkv(key, value, sequence) for (auto&& [key, value] : ni_nkeyvalue_range((sequence)))
#define nrep(index, count) for (int index : ni_ncursor_range<false>(nrange(ni_nloop_count((count)))))
#define nrrep(index, count)                                                                                           \
    for (int index : ni_ncursor_range<false>(nrange(ni_nloop_count((count)) - 1, -1, -1)))

template <class A>
    requires nviewable_indexed<A&&>
constexpr auto nreverse(A&& a) {
    int size = nlen(a);
    return nstride(forward<A>(a), size - 1, size, -1);
}

template <class A, class F>
    requires nviewable_indexed<A&&>
constexpr auto nproject(A&& a, F projection) {
    auto owner = ni::nhold_indexed(forward<A>(a));
    int size = nlen(owner.get());
    return nview(size, [owner = move(owner), projection = move(projection)](int i) -> decltype(auto) {
        return invoke(projection, owner.get()[i]);
    });
}

template <class L, class R> class nzip_view {
    L left_;
    R right_;

  public:
    using nview_tag = void;

    constexpr nzip_view(L left, R right) : left_(move(left)), right_(move(right)) {}
    constexpr int len() const { return min(nlen(left_.get()), nlen(right_.get())); }
    constexpr bool empty() const { return len() == 0; }
    constexpr auto operator[](int i) const {
        npre(0 <= i && i < len());
        using left_reference = decltype(left_.get()[i]);
        using right_reference = decltype(right_.get()[i]);
        return pair<left_reference, right_reference>(left_.get()[i], right_.get()[i]);
    }
};

template <class A, class B>
    requires nviewable_indexed<A&&> && nviewable_indexed<B&&>
constexpr auto nzip(A&& left, B&& right) {
    auto left_holder = ni::nhold_indexed(forward<A>(left));
    auto right_holder = ni::nhold_indexed(forward<B>(right));
    return nzip_view<decltype(left_holder), decltype(right_holder)>(move(left_holder), move(right_holder));
}

template <class L, class R> class nproduct_view {
    L left_;
    R right_;
    int left_size_, right_size_;

  public:
    using nview_tag = void;

    constexpr nproduct_view(L left, R right)
        : left_(move(left)), right_(move(right)), left_size_(nlen(left_.get())),
          right_size_(nlen(right_.get())) {
        npre(left_size_ == 0 || right_size_ <= INT_MAX / left_size_);
    }
    constexpr int len() const { return left_size_ * right_size_; }
    constexpr bool empty() const { return len() == 0; }
    constexpr auto operator[](int i) const {
        npre(0 <= i && i < len());
        int width = right_size_;
        using left_reference = decltype(left_.get()[i / width]);
        using right_reference = decltype(right_.get()[i % width]);
        return pair<left_reference, right_reference>(left_.get()[i / width], right_.get()[i % width]);
    }
};

template <class A, class B>
    requires nviewable_indexed<A&&> && nviewable_indexed<B&&>
constexpr auto nproduct(A&& left, B&& right) {
    auto left_holder = ni::nhold_indexed(forward<A>(left));
    auto right_holder = ni::nhold_indexed(forward<B>(right));
    return nproduct_view<decltype(left_holder), decltype(right_holder)>(move(left_holder), move(right_holder));
}

template <class H> class nwindow_view {
    H owner_;
    int width_, step_, size_;

    static constexpr int count(const H& owner, int width, int step) {
        npre(width >= 0 && step > 0);
        int length = nlen(owner.get());
        if (width > length)
            return 0;
        long long result = 1LL + (length - width) / step;
        npre(result <= INT_MAX);
        return int(result);
    }

  public:
    using nview_tag = void;

    constexpr nwindow_view(H owner, int width, int step)
        : owner_(move(owner)), width_(width), step_(step), size_(count(owner_, width, step)) {}
    constexpr int len() const { return size_; }
    constexpr bool empty() const { return size_ == 0; }
    constexpr auto operator[](int i) const
        requires copy_constructible<H>
    {
        npre(0 <= i && i < size_);
        int first = i * step_;
        H owner = owner_;
        return nview(width_, [owner = move(owner), first](int offset) -> decltype(auto) {
            return owner.get()[first + offset];
        });
    }
};

template <class A>
    requires nviewable_indexed<A&&>
constexpr auto nwindows(A&& owner, int width, int step = 1) {
    auto holder = ni::nhold_indexed(forward<A>(owner));
    return nwindow_view<decltype(holder)>(move(holder), width, step);
}
