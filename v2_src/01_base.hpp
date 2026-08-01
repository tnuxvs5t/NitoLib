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

template <class A> constexpr int nlen(const A& a) {
    if constexpr (requires { a.len(); }) {
        auto n = a.len();
        npre(0 <= n && uint64_t(n) <= uint64_t(INT_MAX));
        return int(n);
    } else {
        auto n = a.size();
        npre(uint64_t(n) <= uint64_t(INT_MAX));
        return int(n);
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
