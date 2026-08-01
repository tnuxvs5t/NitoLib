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

namespace ni {
template <class A>
concept nhas_len_member = requires(const A& a) { a.len(); };

template <class A>
concept nhas_integral_len = requires(const A& a) {
    a.len();
    requires integral<remove_cvref_t<decltype(a.len())>>;
};

template <class A>
concept nhas_integral_size = requires(const A& a) {
    a.size();
    requires integral<remove_cvref_t<decltype(a.size())>>;
};
} // namespace ni

template <class A>
    requires ni::nhas_integral_len<A> ||
             ((!ni::nhas_len_member<A>) && ni::nhas_integral_size<A>)
constexpr int nlen(const A& a) {
    if constexpr (ni::nhas_integral_len<A>) {
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
