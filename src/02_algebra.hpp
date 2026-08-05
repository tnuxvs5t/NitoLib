enum class nlaw : unsigned {
    none = 0,
    associative = 1U << 0,
    identity = 1U << 1,
    inverse = 1U << 2,
    commutative = 1U << 3,
    idempotent = 1U << 4,
};

constexpr nlaw operator|(nlaw a, nlaw b) {
    return nlaw(unsigned(a) | unsigned(b));
}

constexpr bool nhas_law(nlaw laws, nlaw law) {
    return (unsigned(laws) & unsigned(law)) == unsigned(law);
}

template <class O, nlaw Law>
concept ndeclares = requires {
    { O::laws } -> convertible_to<nlaw>;
} && nhas_law(O::laws, Law);

template <class O, class T>
concept nsemigroup = requires(const O& op, T a, const T& b) {
    { op(move(a), b) } -> convertible_to<T>;
} && ndeclares<O, nlaw::associative>;

template <class O, class T>
concept nmonoid = nsemigroup<O, T> && requires(const O& op) {
    { op.id() } -> convertible_to<T>;
} && ndeclares<O, nlaw::identity>;

template <class O, class T>
concept ngroup = nmonoid<O, T> && requires(const O& op, T a) {
    { op.inv(move(a)) } -> convertible_to<T>;
} && ndeclares<O, nlaw::inverse>;

template <class O, class T>
concept ncommutative_monoid = nmonoid<O, T> && ndeclares<O, nlaw::commutative>;

template <class Add, class Multiply, class T>
inline constexpr bool nsemiring_laws = false;

template <class Add, class Multiply, class T>
concept nsemiring = ncommutative_monoid<Add, T> && nmonoid<Multiply, T> &&
                    nsemiring_laws<remove_cvref_t<Add>, remove_cvref_t<Multiply>, T>;

template <class T>
inline constexpr bool nadd_group = is_arithmetic_v<T> && (!same_as<remove_cv_t<T>, bool>);

template <class T> inline constexpr bool nexact_field = false;

template <class T>
concept nexact_field_element = nexact_field<remove_cvref_t<T>> && copyable<remove_cvref_t<T>> &&
                               requires(remove_cvref_t<T> a, const remove_cvref_t<T>& b) {
                                   remove_cvref_t<T>{};
                                   remove_cvref_t<T>{1};
                                   { a == b } -> convertible_to<bool>;
                                   { a != b } -> convertible_to<bool>;
                                   { a * b } -> convertible_to<remove_cvref_t<T>>;
                                   { a *= b } -> same_as<remove_cvref_t<T>&>;
                                   { a -= b } -> same_as<remove_cvref_t<T>&>;
                                   { a / b } -> convertible_to<remove_cvref_t<T>>;
                                   { -a } -> convertible_to<remove_cvref_t<T>>;
                               };

template <class A, class S, class F>
inline constexpr bool naction_laws = false;

template <class A, class S, class F>
concept naction = copyable<F> && naction_laws<remove_cvref_t<A>, S, F> &&
                  requires(const A& action, S aggregate, const F& tag, int length) {
    { action.tag_id() } -> convertible_to<F>;
    { action.compose(tag, tag) } -> convertible_to<F>;
    { action.apply(move(aggregate), tag, length) } -> convertible_to<S>;
};

template <class T> struct nadd {
    static constexpr nlaw laws = nlaw::associative | nlaw::identity |
                                 (nadd_group<T> ? nlaw::inverse | nlaw::commutative : nlaw::none);
    constexpr T id() const
        requires default_initializable<T>
    {
        return T{};
    }
    constexpr T operator()(T a, const T& b) const
        requires requires(T x, const T& y) {
            { x += y } -> same_as<T&>;
        }
    {
        return a += b;
    }
    constexpr T inv(T a) const
        requires requires(T x) {
            { -x } -> convertible_to<T>;
        }
    {
        return -a;
    }
};

template <class T> struct nmul {
    static constexpr nlaw laws = nlaw::associative | nlaw::identity;
    constexpr T id() const
        requires requires { T{1}; }
    {
        return T{1};
    }
    constexpr T operator()(T a, const T& b) const
        requires requires(T x, const T& y) {
            { x *= y } -> same_as<T&>;
        }
    {
        return a *= b;
    }
};

template <class T> struct nxor {
    static constexpr nlaw laws =
        nlaw::associative | nlaw::identity | nlaw::inverse | nlaw::commutative;
    constexpr T id() const
        requires default_initializable<T>
    {
        return T{};
    }
    constexpr T operator()(T a, const T& b) const
        requires requires(T x, const T& y) {
            { x ^= y } -> same_as<T&>;
        }
    {
        return a ^= b;
    }
    constexpr T inv(T a) const { return a; }
};

template <class T> struct nmin {
    static constexpr nlaw laws =
        nlaw::associative | nlaw::identity | nlaw::commutative | nlaw::idempotent;
    constexpr T id() const
        requires numeric_limits<T>::is_specialized
    {
        if constexpr (numeric_limits<T>::has_infinity)
            return numeric_limits<T>::infinity();
        else
            return numeric_limits<T>::max();
    }
    constexpr T operator()(const T& a, const T& b) const
        requires requires { b < a; }
    {
        return b < a ? b : a;
    }
};

template <class T> struct nmax {
    static constexpr nlaw laws =
        nlaw::associative | nlaw::identity | nlaw::commutative | nlaw::idempotent;
    constexpr T id() const
        requires numeric_limits<T>::is_specialized
    {
        if constexpr (numeric_limits<T>::has_infinity)
            return -numeric_limits<T>::infinity();
        else
            return numeric_limits<T>::lowest();
    }
    constexpr T operator()(const T& a, const T& b) const
        requires requires { a < b; }
    {
        return a < b ? b : a;
    }
};

template <class T> struct naddsum_action {
    constexpr T tag_id() const { return T{}; }
    constexpr T compose(const T& newer, const T& older) const { return older + newer; }
    constexpr T apply(T sum, const T& delta, int length) const { return sum + delta * T(length); }
};

template <integral T>
    requires(!same_as<remove_cv_t<T>, bool>)
inline constexpr bool naction_laws<naddsum_action<T>, T, T> = true;

template <integral T>
    requires(!same_as<remove_cv_t<T>, bool>)
inline constexpr bool nsemiring_laws<nadd<T>, nmul<T>, T> = true;

template <class T, class O = nmul<T>>
    requires nmonoid<O, T> && copy_constructible<T>
constexpr T npow(T base, long long exponent, O operation = {}) {
    uint64_t remaining;
    if (exponent < 0) {
        if constexpr (ngroup<O, T>) {
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
