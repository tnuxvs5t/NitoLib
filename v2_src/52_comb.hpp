template <unsigned_integral T> class nsubmask_range {
    T mask_{};

  public:
    explicit constexpr nsubmask_range(T mask) : mask_(mask) {
        npre(popcount(mask) <= 30);
    }

    constexpr int len() const { return int(T(1) << popcount(mask_)); }
    constexpr bool empty() const { return false; }

    struct cursor {
        T mask{}, value{};
        int index = 0;
        bool active = true;

        constexpr bool ok() const { return active; }
        constexpr T val() const { return value; }
        constexpr int idx() const { return index; }
        constexpr void next() {
            if (!value)
                active = false;
            else {
                value = (value - 1) & mask;
                ++index;
            }
        }
    };

    constexpr cursor enumerate() const { return {mask_, mask_}; }
};

template <unsigned_integral T> constexpr auto nsubmasks(T mask) { return nsubmask_range<T>(mask); }

namespace ni {
template <class A> constexpr void nsubset_extent(const A& a) {
    int n = nlen(a);
    npre(n > 0 && has_single_bit(unsigned(n)));
}

template <nindexed A> auto nindex_copy(const A& source) {
    using T = nindex_value_t<const A>;
    nvector<T> result(nlen(source));
    for (int i = 0; i < nlen(source); ++i)
        result[i] = source[i];
    return result;
}
} // namespace ni

template <class A>
    requires nreference_indexed<A> && requires(nindex_reference_t<A> x, nindex_value_t<A> y) { x += y; }
void nzeta_subset(A& a) {
    ni::nsubset_extent(a);
    for (int bit = 1; bit < nlen(a); bit <<= 1)
        for (int mask = 0; mask < nlen(a); ++mask)
            if (mask & bit)
                a[mask] += a[mask ^ bit];
}

template <class A>
    requires nreference_indexed<A> && requires(nindex_reference_t<A> x, nindex_value_t<A> y) { x -= y; }
void nmobius_subset(A& a) {
    ni::nsubset_extent(a);
    for (int bit = 1; bit < nlen(a); bit <<= 1)
        for (int mask = 0; mask < nlen(a); ++mask)
            if (mask & bit)
                a[mask] -= a[mask ^ bit];
}

template <class A>
    requires nreference_indexed<A> && requires(nindex_reference_t<A> x, nindex_value_t<A> y) { x += y; }
void nzeta_superset(A& a) {
    ni::nsubset_extent(a);
    for (int bit = 1; bit < nlen(a); bit <<= 1)
        for (int mask = 0; mask < nlen(a); ++mask)
            if (!(mask & bit))
                a[mask] += a[mask | bit];
}

template <class A>
    requires nreference_indexed<A> && requires(nindex_reference_t<A> x, nindex_value_t<A> y) { x -= y; }
void nmobius_superset(A& a) {
    ni::nsubset_extent(a);
    for (int bit = 1; bit < nlen(a); bit <<= 1)
        for (int mask = 0; mask < nlen(a); ++mask)
            if (!(mask & bit))
                a[mask] -= a[mask | bit];
}

template <class A>
    requires nreference_indexed<A> && requires(nindex_value_t<A> x, nindex_reference_t<A> y, int n) {
        y = x + x;
        y = x - x;
        y /= n;
    }
void nfwht_xor(A& a, bool inverse = false) {
    ni::nsubset_extent(a);
    for (int width = 1; width < nlen(a); width <<= 1)
        for (int first = 0; first < nlen(a); first += width << 1)
            for (int offset = 0; offset < width; ++offset) {
                auto x = a[first + offset], y = a[first + width + offset];
                a[first + offset] = x + y;
                a[first + width + offset] = x - y;
            }
    if (inverse)
        for (int i = 0; i < nlen(a); ++i)
            a[i] /= nlen(a);
}

template <nindexed A, nindexed B> auto nconv_or(const A& a, const B& b) {
    npre(nlen(a) == nlen(b));
    auto left = ni::nindex_copy(a), right = ni::nindex_copy(b);
    nzeta_subset(left);
    nzeta_subset(right);
    for (int i = 0; i < left.len(); ++i)
        left[i] *= right[i];
    nmobius_subset(left);
    return left;
}

template <nindexed A, nindexed B> auto nconv_and(const A& a, const B& b) {
    npre(nlen(a) == nlen(b));
    auto left = ni::nindex_copy(a), right = ni::nindex_copy(b);
    nzeta_superset(left);
    nzeta_superset(right);
    for (int i = 0; i < left.len(); ++i)
        left[i] *= right[i];
    nmobius_superset(left);
    return left;
}

template <nindexed A, nindexed B> auto nconv_xor(const A& a, const B& b) {
    npre(nlen(a) == nlen(b));
    auto left = ni::nindex_copy(a), right = ni::nindex_copy(b);
    nfwht_xor(left);
    nfwht_xor(right);
    for (int i = 0; i < left.len(); ++i)
        left[i] *= right[i];
    nfwht_xor(left, true);
    return left;
}
