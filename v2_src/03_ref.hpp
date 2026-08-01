template <class T> class nspan {
    T* data_ = nullptr;
    int size_ = 0;

  public:
    using value_type = remove_cv_t<T>;
    using reference = T&;

    constexpr nspan() = default;
    constexpr nspan(T* data, int size) : data_(data), size_(size) {
        npre(size >= 0);
        npre(size == 0 || data != nullptr);
    }

    template <size_t N> constexpr nspan(T (&data)[N]) : data_(data), size_(int(N)) {
        static_assert(N <= size_t(INT_MAX));
    }

    template <class U>
        requires is_convertible_v<U (*)[], T (*)[]>
    constexpr nspan(const nspan<U>& other) : data_(other.data()), size_(other.len()) {}

    constexpr int len() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }
    constexpr T* data() const noexcept { return data_; }

    constexpr T& operator[](int i) const {
        npre(0 <= i && i < size_);
        return data_[i];
    }
    constexpr T* get(int i) const noexcept { return 0 <= i && i < size_ ? data_ + i : nullptr; }

    constexpr nspan sub(int l, int r) const {
        npre(0 <= l && l <= r && r <= size_);
        return {l ? data_ + l : data_, r - l};
    }
    constexpr nspan sub(int l) const { return sub(l, size_); }
};

template <class F> class nview {
    int size_ = 0;
    [[no_unique_address]] F access_;

  public:
    using accessor_type = F;

    constexpr nview(int size, F access) : size_(size), access_(move(access)) { npre(size >= 0); }

    constexpr int len() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }

    constexpr decltype(auto) operator[](int i) {
        npre(0 <= i && i < size_);
        return invoke(access_, i);
    }
    constexpr decltype(auto) operator[](int i) const
        requires invocable<const F&, int>
    {
        npre(0 <= i && i < size_);
        return invoke(access_, i);
    }
};

template <class F> nview(int, F) -> nview<F>;

template <class A>
concept nindexed = requires(A& a, const A& ca, int i) {
    { nlen(ca) } -> same_as<int>;
    a[i];
    ca[i];
};

template <class A> using nindex_reference_t = decltype(declval<A&>()[0]);
template <class A> using nindex_value_t = remove_cvref_t<nindex_reference_t<A>>;

template <class A>
concept nreference_indexed = nindexed<A> && is_lvalue_reference_v<nindex_reference_t<A>>;

template <class A>
concept nswappable_indexed = nreference_indexed<A> &&
                             (!is_const_v<remove_reference_t<nindex_reference_t<A>>>) && requires(A& a) {
                                 ranges::swap(a[0], a[0]);
                             };

template <class A>
concept ncontiguous_indexed = nindexed<A> && requires(A& a) {
    { a.data() } -> contiguous_iterator;
};

template <class A>
concept nresizable = requires(A& a, int n) {
    a.resize(n);
};

namespace ni {
template <class A> struct nindex_access {
    A* owner;
    constexpr decltype(auto) operator()(int i) const { return (*owner)[i]; }
};
} // namespace ni

template <class A> constexpr auto nall(A& a) {
    return nview(nlen(a), ni::nindex_access<A>{addressof(a)});
}
template <class A> auto nall(A&&) = delete;

template <class A> constexpr auto nsub(A& a, int l, int r) {
    npre(0 <= l && l <= r && r <= nlen(a));
    return nview(r - l, [owner = addressof(a), l](int i) -> decltype(auto) { return (*owner)[l + i]; });
}

template <class F> constexpr auto nsub(nview<F> view, int l, int r) {
    npre(0 <= l && l <= r && r <= view.len());
    return nview(r - l, [view = move(view), l](int i) -> decltype(auto) { return view[l + i]; });
}

template <class A> constexpr auto nstride(A& a, int first, int count, int step) {
    npre(count >= 0);
    if (count) {
        long long last = first + 1LL * (count - 1) * step;
        npre(0 <= first && first < nlen(a));
        npre(0 <= last && last < nlen(a));
    }
    return nview(count, [owner = addressof(a), first, step](int i) -> decltype(auto) {
        return (*owner)[int(first + 1LL * i * step)];
    });
}
