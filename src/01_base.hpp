inline constexpr int npos = -1;

template <class T>
using nwide_t = conditional_t<is_integral_v<T>, __int128_t, long double>;

template <class T>
inline constexpr T ninf = [] {
    if constexpr (numeric_limits<T>::has_infinity)
        return numeric_limits<T>::infinity();
    else
        return numeric_limits<T>::max() / 4;
}();

template <class T>
inline constexpr T nninf = [] {
    if constexpr (numeric_limits<T>::has_infinity)
        return -numeric_limits<T>::infinity();
    else if constexpr (is_signed_v<T>)
        return numeric_limits<T>::lowest() / 4;
    else
        return T{};
}();

namespace ni {
template <integral To, integral From> constexpr To nchecked_integral_cast(From value) {
    if constexpr (signed_integral<From> && signed_integral<To>) {
        npre(__int128_t(value) >= __int128_t(numeric_limits<To>::lowest()));
        npre(__int128_t(value) <= __int128_t(numeric_limits<To>::max()));
    } else if constexpr (signed_integral<From>) {
        npre(value >= 0);
        npre(__uint128_t(value) <= __uint128_t(numeric_limits<To>::max()));
    } else {
        npre(__uint128_t(value) <= __uint128_t(numeric_limits<To>::max()));
    }
    return To(value);
}

template <integral I> constexpr int nchecked_int(I value) {
    return nchecked_integral_cast<int>(value);
}

template <class To, class From>
    requires is_arithmetic_v<To> && is_arithmetic_v<From>
constexpr To nchecked_number(From value) {
    if constexpr (integral<To> && integral<From>) {
        return nchecked_integral_cast<To>(value);
    } else {
        long double wide = static_cast<long double>(value);
        npre(!isnan(wide));
        if constexpr (integral<To>) {
            npre(isfinite(wide) && trunc(wide) == wide);
            long double limit = ldexp(1.0L, numeric_limits<To>::digits);
            if constexpr (signed_integral<To>)
                npre(-limit <= wide && wide < limit);
            else
                npre(0 <= wide && wide < limit);
        } else if (isfinite(wide)) {
            npre(wide >= -static_cast<long double>(numeric_limits<To>::max()));
            npre(wide <= static_cast<long double>(numeric_limits<To>::max()));
        } else {
            npre(numeric_limits<To>::has_infinity);
        }
        return static_cast<To>(value);
    }
}

template <class T> constexpr T nchecked_add(T a, T b) {
    if constexpr (is_integral_v<T>) {
        T result;
        npre(!__builtin_add_overflow(a, b, &result));
        return result;
    } else {
        return a + b;
    }
}

template <class T> constexpr T nchecked_sub(T a, T b) {
    if constexpr (is_integral_v<T>) {
        T result;
        npre(!__builtin_sub_overflow(a, b, &result));
        return result;
    } else {
        return a - b;
    }
}

template <class T> constexpr T nchecked_mul(T a, T b) {
    if constexpr (is_integral_v<T>) {
        T result;
        npre(!__builtin_mul_overflow(a, b, &result));
        return result;
    } else {
        return a * b;
    }
}
} // namespace ni

template <class T> class nmaybe {
    optional<T> value_;

  public:
    constexpr nmaybe() = default;
    constexpr nmaybe(const T& value) : value_(value) {}
    constexpr nmaybe(T&& value) : value_(move(value)) {}

    constexpr bool ok() const noexcept { return value_.has_value(); }
    constexpr explicit operator bool() const noexcept { return ok(); }

    constexpr T& val() {
        npre(ok());
        return *value_;
    }
    constexpr const T& val() const {
        npre(ok());
        return *value_;
    }
    constexpr T val(T fallback) const& { return value_ ? *value_ : move(fallback); }
    constexpr T val(T fallback) && { return value_ ? move(*value_) : move(fallback); }

    constexpr T& operator*() { return val(); }
    constexpr const T& operator*() const { return val(); }
    constexpr T* operator->() { return addressof(val()); }
    constexpr const T* operator->() const { return addressof(val()); }
    constexpr void reset() noexcept { value_.reset(); }
};

template <class T, class U> constexpr bool nchmin(T& a, U&& b) {
    if (b < a) {
        a = forward<U>(b);
        return true;
    }
    return false;
}

template <class T, class U> constexpr bool nchmax(T& a, U&& b) {
    if (a < b) {
        a = forward<U>(b);
        return true;
    }
    return false;
}

template <class A>
    requires requires(const A& value) {
        { value.len() } -> integral;
    } || ((!requires(const A& value) { value.len(); }) && requires(const A& value) {
        { value.size() } -> integral;
    })
constexpr int nlen(const A& a) {
    if constexpr (requires(const A& value) {
                      { value.len() } -> integral;
                  }) {
        auto n = a.len();
        if constexpr (signed_integral<remove_cvref_t<decltype(n)>>)
            npre(n >= 0);
        return ni::nchecked_int(n);
    } else {
        auto n = a.size();
        if constexpr (signed_integral<remove_cvref_t<decltype(n)>>)
            npre(n >= 0);
        return ni::nchecked_int(n);
    }
}

constexpr int nbitceil(int n) {
    npre(0 <= n && n <= (1 << 30));
    return n <= 1 ? 1 : int(bit_ceil(unsigned(n)));
}

template <class T = void> struct nless {
    constexpr bool operator()(const auto& a, const auto& b) const { return a < b; }
};

template <class T = void> struct ngreater {
    constexpr bool operator()(const auto& a, const auto& b) const { return b < a; }
};

template <class T = void> struct nequal {
    constexpr bool operator()(const auto& a, const auto& b) const { return a == b; }
};

struct nidentity {
    template <class T> constexpr T&& operator()(T&& value) const noexcept {
        return forward<T>(value);
    }
};

// Operation objects are deliberately syntax-light: users must ensure `id()` is a
// two-sided identity and `operator()` is associative on the values actually used.
// `inv()` is meaningful only where every queried value has a true inverse; signed
// arithmetic additionally requires that no operation overflows.
template <class T> struct nadd {
    constexpr T id() const { return T{}; }
    constexpr T operator()(T left, const T& right) const { return left += right; }
    constexpr T inv(T value) const { return -value; }
};

template <class T> struct nmul {
    constexpr T id() const { return T{1}; }
    constexpr T operator()(T left, const T& right) const { return left *= right; }
};

template <class T> struct nxor {
    constexpr T id() const { return T{}; }
    constexpr T operator()(T left, const T& right) const { return left ^= right; }
    constexpr T inv(T value) const { return value; }
};

template <class T> struct nmin {
    constexpr T id() const {
        if constexpr (numeric_limits<T>::has_infinity)
            return numeric_limits<T>::infinity();
        else
            return numeric_limits<T>::max();
    }
    constexpr T operator()(const T& left, const T& right) const {
        return right < left ? right : left;
    }
};

template <class T> struct nmax {
    constexpr T id() const {
        if constexpr (numeric_limits<T>::has_infinity)
            return -numeric_limits<T>::infinity();
        else
            return numeric_limits<T>::lowest();
    }
    constexpr T operator()(const T& left, const T& right) const {
        return left < right ? right : left;
    }
};

// SplitMix-style deterministic generator.  It is not cryptographic; reproducibility
// requires explicit seeding and must not depend on unspecified call ordering.
class nrng {
    uint64_t state_;

  public:
    using result_type = uint64_t;

    constexpr explicit nrng(uint64_t seed = 0x243f6a8885a308d3ULL) : state_(seed) {}
    static constexpr uint64_t min() noexcept { return 0; }
    static constexpr uint64_t max() noexcept { return numeric_limits<uint64_t>::max(); }
    static constexpr uint64_t mix(uint64_t value) noexcept {
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31);
    }

    constexpr uint64_t operator()() noexcept { return state_ = mix(state_); }

    template <integral I> constexpr I operator()(I bound) {
        npre(bound > 0);
        using U = make_unsigned_t<I>;
        return I((__uint128_t((*this)()) * U(bound)) >> 64);
    }

    template <integral I> constexpr I operator()(I first, I last) {
        npre(first < last);
        using U = make_unsigned_t<I>;
        U width = U(last) - U(first);
        if constexpr (signed_integral<I>)
            return I(__int128_t(first) + __int128_t((*this)(width)));
        else
            return I(U(first) + (*this)(width));
    }
};

inline uint64_t nseed_value =
    uint64_t(chrono::steady_clock::now().time_since_epoch().count()) ^
    uint64_t(reinterpret_cast<uintptr_t>(addressof(nseed_value)));
inline nrng nrng_global(nseed_value);
inline uint64_t nhash_seed = nrng::mix(nseed_value);

inline void nseed(uint64_t seed) {
    nseed_value = seed;
    nrng_global = nrng(seed);
    nhash_seed = nrng::mix(seed);
}

// Salted contest hash.  Equal keys must hash equally; the process salt intentionally
// changes bucket behavior unless nseed is called before container construction.
template <class T> struct nhash {
    uint64_t salt = nhash_seed;

    nhash() = default;
    explicit nhash(uint64_t seed) : salt(seed) {}
    size_t operator()(const T& value) const noexcept(noexcept(hash<T>{}(value))) {
        return size_t(nrng::mix(uint64_t(hash<T>{}(value)) + salt));
    }
};

template <class A, class B> struct nhash<pair<A, B>> {
    uint64_t salt = nhash_seed;

    nhash() = default;
    explicit nhash(uint64_t seed) : salt(seed) {}
    size_t operator()(const pair<A, B>& value) const
        noexcept(noexcept(nhash<A>{salt}(value.first)) &&
                 noexcept(nhash<B>{salt ^ 0x9e3779b97f4a7c15ULL}(value.second))) {
        uint64_t left = uint64_t(nhash<A>{salt}(value.first));
        uint64_t right = uint64_t(nhash<B>{salt ^ 0x9e3779b97f4a7c15ULL}(value.second));
        return size_t(nrng::mix(left ^ (right + 0x9e3779b97f4a7c15ULL)));
    }
};
