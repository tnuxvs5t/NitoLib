/**
 * Half-open arithmetic range [first,last) with nonzero step.  The caller must choose
 * a step whose sign reaches the bound; constructor arithmetic is checked for overflow.
 */
template <signed_integral T> class nrange_t {
    T first_ = 0, last_ = 0, step_ = 1;

  public:
    using value_type = T;
    using nrange_tag = void;

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
    constexpr int position(T value) const {
        __int128_t distance = step_ > 0 ? __int128_t(value) - __int128_t(first_)
                                        : __int128_t(first_) - __int128_t(value);
        if (distance < 0)
            return npos;
        __int128_t stride = step_ > 0 ? __int128_t(step_) : -__int128_t(step_);
        if (distance % stride != 0)
            return npos;
        __int128_t index = distance / stride;
        return index < len() ? int(index) : npos;
    }
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
// Single-pass cursor borrowing indexed storage.  The source must outlive enumeration
// and retain length/index validity until the cursor reaches the end.
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
    return nstride(forward<A>(a), size - 1, -1, -1);
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

namespace ni {
// Zip/product/window accessors preserve holder lifetime rules.  Zip exposes the common
// supported prefix; windows borrow overlapping elements rather than copying them.
template <class L, class R> class nzip_access {
    L left_;
    R right_;

    template <class LH, class RH> static constexpr auto get(LH& left, RH& right, int index) {
        using left_reference = decltype(left.get()[index]);
        using right_reference = decltype(right.get()[index]);
        return pair<left_reference, right_reference>(left.get()[index], right.get()[index]);
    }

  public:
    constexpr nzip_access(L left, R right) : left_(move(left)), right_(move(right)) {}
    constexpr auto operator()(int index) { return get(left_, right_, index); }
    constexpr auto operator()(int index) const
        requires requires(const L& left, const R& right) {
            left.get()[0];
            right.get()[0];
        }
    {
        return get(left_, right_, index);
    }
};

template <class L, class R> class nproduct_access {
    L left_;
    R right_;
    int right_size_ = 0;

    template <class LH, class RH>
    static constexpr auto get(LH& left, RH& right, int width, int index) {
        using left_reference = decltype(left.get()[index / width]);
        using right_reference = decltype(right.get()[index % width]);
        return pair<left_reference, right_reference>(left.get()[index / width],
                                                     right.get()[index % width]);
    }

  public:
    constexpr nproduct_access(L left, R right, int right_size)
        : left_(move(left)), right_(move(right)), right_size_(right_size) {}
    constexpr auto operator()(int index) { return get(left_, right_, right_size_, index); }
    constexpr auto operator()(int index) const
        requires requires(const L& left, const R& right) {
            left.get()[0];
            right.get()[0];
        }
    {
        return get(left_, right_, right_size_, index);
    }
};

template <class H> class nwindow_access {
    H owner_;
    int width_ = 0, step_ = 1;

    template <class Owner> static constexpr auto get(Owner owner, int width, int step, int index) {
        using access_type = nsub_access<Owner>;
        return nview(width, access_type(move(owner), index * step));
    }

  public:
    constexpr nwindow_access(H owner, int width, int step)
        : owner_(move(owner)), width_(width), step_(step) {}
    constexpr auto operator()(int index)
        requires copy_constructible<H>
    {
        return get(owner_, width_, step_, index);
    }
    constexpr auto operator()(int index) const
        requires copy_constructible<H>
    {
        return get(owner_, width_, step_, index);
    }
};

constexpr int nwindow_count(int length, int width, int step) {
    npre(width >= 0 && step > 0);
    if (width > length)
        return 0;
    long long result = 1LL + (length - width) / step;
    npre(result <= INT_MAX);
    return int(result);
}
} // namespace ni

template <class A, class B>
    requires nviewable_indexed<A&&> && nviewable_indexed<B&&>
constexpr auto nzip(A&& left, B&& right) {
    auto left_holder = ni::nhold_indexed(forward<A>(left));
    auto right_holder = ni::nhold_indexed(forward<B>(right));
    int size = min(nlen(left_holder.get()), nlen(right_holder.get()));
    using access_type = ni::nzip_access<decltype(left_holder), decltype(right_holder)>;
    return nview(size, access_type(move(left_holder), move(right_holder)));
}

template <class A, class B>
    requires nviewable_indexed<A&&> && nviewable_indexed<B&&>
constexpr auto nproduct(A&& left, B&& right) {
    auto left_holder = ni::nhold_indexed(forward<A>(left));
    auto right_holder = ni::nhold_indexed(forward<B>(right));
    int left_size = nlen(left_holder.get()), right_size = nlen(right_holder.get());
    npre(left_size == 0 || right_size <= INT_MAX / left_size);
    using access_type = ni::nproduct_access<decltype(left_holder), decltype(right_holder)>;
    return nview(left_size * right_size,
                 access_type(move(left_holder), move(right_holder), right_size));
}

template <class A>
    requires nviewable_indexed<A&&>
constexpr auto nwindows(A&& owner, int width, int step = 1) {
    auto holder = ni::nhold_indexed(forward<A>(owner));
    int size = ni::nwindow_count(nlen(holder.get()), width, step);
    using access_type = ni::nwindow_access<decltype(holder)>;
    return nview(size, access_type(move(holder), width, step));
}
