template <class T> struct nntt_info {
    static constexpr bool ok = false;
};
template <> struct nntt_info<nmodint<998244353>> {
    static constexpr bool ok = true;
    static constexpr uint64_t root = 3;
};
template <> struct nntt_info<nmodint<1004535809>> {
    static constexpr bool ok = true;
    static constexpr uint64_t root = 3;
};
template <> struct nntt_info<nmodint<469762049>> {
    static constexpr bool ok = true;
    static constexpr uint64_t root = 3;
};

namespace ni {
template <class T> inline constexpr bool nstatic_modular = false;
template <uint64_t Modulus> inline constexpr bool nstatic_modular<nmodint<Modulus>> = true;

inline uint64_t nprimitive_root(uint64_t modulus) {
    npre(2 <= modulus && modulus <= UINT32_MAX && nisprime(modulus));
    if (modulus == 2)
        return 1;
    uint64_t phi = modulus - 1, remaining = phi;
    nvector<uint64_t> factors;
    for (uint64_t prime = 2; prime <= remaining / prime; ++prime)
        if (remaining % prime == 0) {
            factors.push(prime);
            while (remaining % prime == 0)
                remaining /= prime;
        }
    if (remaining > 1)
        factors.push(remaining);
    for (uint64_t candidate = 2;; ++candidate) {
        bool primitive = true;
        for (int i = 0; i < factors.len(); ++i)
            if (npowmod(candidate, phi / factors[i], modulus) == 1) {
                primitive = false;
                break;
            }
        if (primitive)
            return candidate;
    }
}

template <uint64_t Modulus> void nntt(nvector<nmodint<Modulus>>& values, bool inverse) {
    int n = values.len();
    npre(n > 0 && has_single_bit(unsigned(n)));
    npre((Modulus - 1) % uint64_t(n) == 0);
    for (int i = 1, reversed = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; reversed & bit; bit >>= 1)
            reversed ^= bit;
        reversed ^= bit;
        if (i < reversed)
            swap(values[i], values[reversed]);
    }
    using mint = nmodint<Modulus>;
    static const uint64_t primitive = [] {
        if constexpr (nntt_info<mint>::ok)
            return nntt_info<mint>::root;
        else
            return nprimitive_root(Modulus);
    }();
    for (int width = 2; width <= n;) {
        mint step = mint(primitive).pow((Modulus - 1) / uint64_t(width));
        if (inverse)
            step = step.inverse().val();
        for (int first = 0; first < n; first += width) {
            mint root = 1;
            for (int offset = 0; offset < width / 2; ++offset) {
                mint even = values[first + offset];
                mint odd = values[first + offset + width / 2] * root;
                values[first + offset] = even + odd;
                values[first + offset + width / 2] = even - odd;
                root *= step;
            }
        }
        if (width == n)
            break;
        width <<= 1;
    }
    if (inverse) {
        mint scale = mint(n).inverse().val();
        for (int i = 0; i < n; ++i)
            values[i] *= scale;
    }
}
} // namespace ni

template <nindexed A, nindexed B> auto nconv_naive(const A& a, const B& b) {
    using T = nindex_value_t<const A>;
    static_assert(same_as<T, nindex_value_t<const B>>);
    if (!nlen(a) || !nlen(b))
        return nvector<T>{};
    npre(nlen(a) <= INT_MAX - nlen(b) + 1);
    nvector<T> result(nlen(a) + nlen(b) - 1);
    for (int i = 0; i < nlen(a); ++i)
        for (int j = 0; j < nlen(b); ++j)
            result[i + j] += a[i] * b[j];
    return result;
}

template <nindexed A, nindexed B>
    requires ni::nstatic_modular<nindex_value_t<const A>> &&
             same_as<nindex_value_t<const A>, nindex_value_t<const B>>
auto nconv_ntt(const A& a, const B& b) {
    using mint = nindex_value_t<const A>;
    npre(nisprime(mint::mod()));
    if (!nlen(a) || !nlen(b))
        return nvector<mint>{};
    npre(nlen(a) <= INT_MAX - nlen(b) + 1);
    int size = nlen(a) + nlen(b) - 1, transform_size = nbitceil(size);
    npre(mint::mod() <= UINT32_MAX && (mint::mod() - 1) % uint64_t(transform_size) == 0);
    nvector<mint> left(transform_size), right(transform_size);
    for (int i = 0; i < nlen(a); ++i)
        left[i] = a[i];
    for (int i = 0; i < nlen(b); ++i)
        right[i] = b[i];
    ni::nntt(left, false);
    ni::nntt(right, false);
    for (int i = 0; i < transform_size; ++i)
        left[i] *= right[i];
    ni::nntt(left, true);
    left.resize(size);
    return left;
}

template <nindexed A, nindexed B> auto nconv_auto(const A& a, const B& b) {
    using T = nindex_value_t<const A>;
    static_assert(same_as<T, nindex_value_t<const B>>);
    if constexpr (ni::nstatic_modular<T>) {
        if (nlen(a) && nlen(b) && min(nlen(a), nlen(b)) >= 32) {
            npre(nlen(a) <= INT_MAX - nlen(b) + 1);
            long long size = 1LL * nlen(a) + nlen(b) - 1;
            if (size <= (1 << 30)) {
                int transform_size = nbitceil(int(size));
                if (T::mod() <= UINT32_MAX && nisprime(T::mod()) &&
                    (T::mod() - 1) % uint64_t(transform_size) == 0)
                    return nconv_ntt(a, b);
            }
        }
    }
    return nconv_naive(a, b);
}

template <nindexed A, nindexed B> auto nconv(const A& a, const B& b) { return nconv_auto(a, b); }

template <nindexed A> auto npoly_derivative(const A& polynomial) {
    using T = nindex_value_t<const A>;
    nvector<T> result(max(0, nlen(polynomial) - 1));
    for (int i = 1; i < nlen(polynomial); ++i)
        result[i - 1] = polynomial[i] * T(i);
    return result;
}

template <nindexed A> auto npoly_integral(const A& polynomial) {
    using T = nindex_value_t<const A>;
    npre(nlen(polynomial) < INT_MAX);
    nvector<T> result(nlen(polynomial) + 1);
    for (int i = 0; i < nlen(polynomial); ++i)
        result[i + 1] = polynomial[i] / T(i + 1);
    return result;
}

template <nindexed A, class X> auto npoly_evaluate(const A& polynomial, const X& point) {
    using T = nindex_value_t<const A>;
    T result{};
    for (int i = nlen(polynomial); i-- > 0;)
        result = result * point + polynomial[i];
    return result;
}

template <nindexed A> auto nfps_inverse(const A& series, int terms) {
    using T = nindex_value_t<const A>;
    npre(terms >= 0);
    if (!terms)
        return nvector<T>{};
    npre(nlen(series) > 0 && series[0] != T{});
    nvector<T> result{T{1} / series[0]};
    while (result.len() < terms) {
        int size = int(min<long long>(terms, 2LL * result.len()));
        nvector<T> prefix(size);
        for (int i = 0; i < min(size, nlen(series)); ++i)
            prefix[i] = series[i];
        auto correction = nconv(prefix, result);
        correction.resize(size);
        correction[0] = T{2} - correction[0];
        for (int i = 1; i < size; ++i)
            correction[i] = -correction[i];
        result = nconv(result, correction);
        result.resize(size);
    }
    return result;
}

template <class T> class npoly {
  public:
    using value_type = T;
    nvector<T> a;

    npoly() = default;
    npoly(initializer_list<T> coefficients) : a(coefficients) { norm(); }
    explicit npoly(nvector<T> coefficients) : a(move(coefficients)) { norm(); }

    int len() const noexcept { return a.len(); }
    int deg() const noexcept { return len() - 1; }
    bool empty() const noexcept { return a.empty(); }
    void norm() {
        while (!a.empty() && a.back() == T{})
            a.pop();
    }
    T operator[](int index) const { return 0 <= index && index < len() ? a[index] : T{}; }
    T& at(int index) {
        npre(0 <= index && index < len());
        return a[index];
    }
    const T& at(int index) const {
        npre(0 <= index && index < len());
        return a[index];
    }
    template <class X> T operator()(const X& point) const {
        T result{};
        for (int index = len(); index-- > 0;)
            result = result * point + a[index];
        return result;
    }

    npoly& operator+=(const npoly& other) {
        if (len() < other.len())
            a.resize(other.len(), T{});
        for (int index = 0; index < other.len(); ++index)
            a[index] += other.a[index];
        norm();
        return *this;
    }
    npoly& operator-=(const npoly& other) {
        if (len() < other.len())
            a.resize(other.len(), T{});
        for (int index = 0; index < other.len(); ++index)
            a[index] -= other.a[index];
        norm();
        return *this;
    }
    npoly& operator*=(const npoly& other) {
        a = nconv(a, other.a);
        norm();
        return *this;
    }
    friend npoly operator+(npoly left, const npoly& right) { return left += right; }
    friend npoly operator-(npoly left, const npoly& right) { return left -= right; }
    friend npoly operator*(npoly left, const npoly& right) { return left *= right; }
    friend bool operator==(const npoly&, const npoly&) = default;

    npoly deriv() const {
        if (len() < 2)
            return {};
        nvector<T> result(len() - 1);
        for (int index = 1; index < len(); ++index)
            result[index - 1] = a[index] * T(index);
        return npoly(move(result));
    }
    npoly integral() const {
        npre(len() < INT_MAX);
        nvector<T> result(len() + 1, T{});
        for (int index = 0; index < len(); ++index)
            result[index + 1] = a[index] / T(index + 1);
        return npoly(move(result));
    }
    npoly cut(int terms) const {
        npre(terms >= 0);
        nvector<T> result(min(terms, len()));
        for (int index = 0; index < result.len(); ++index)
            result[index] = a[index];
        return npoly(move(result));
    }
    npoly inv(int terms) const {
        npre(terms >= 0);
        if (!terms)
            return {};
        npre(!empty() && a[0] != T{});
        return npoly(nfps_inverse(a, terms));
    }
    npoly log(int terms) const {
        npre(terms >= 0);
        if (!terms)
            return {};
        npre((*this)[0] == T{1});
        return (deriv() * inv(terms)).cut(terms - 1).integral().cut(terms);
    }
    npoly exp(int terms) const {
        npre(terms >= 0);
        if (!terms)
            return {};
        npre((*this)[0] == T{});
        npoly result{T{1}};
        for (int size = 1; size < terms;) {
            int next = int(min<long long>(terms, 2LL * size));
            npoly correction = cut(next) - result.log(next);
            if (correction.empty())
                correction.a.resize(1);
            correction.a[0] += T{1};
            result = (result * correction).cut(next);
            size = next;
        }
        return result.cut(terms);
    }
};

template <nindexed A> auto nberlekamp(const A& sequence) {
    using T = nindex_value_t<const A>;
    nvector<T> current{T{1}}, previous{T{1}};
    int length = 0, shift = 1;
    T previous_discrepancy{1};
    for (int index = 0; index < nlen(sequence); ++index) {
        T discrepancy = sequence[index];
        for (int i = 1; i <= length; ++i)
            discrepancy += current[i] * sequence[index - i];
        if (discrepancy == T{}) {
            ++shift;
            continue;
        }
        nvector<T> saved = current;
        T factor = discrepancy / previous_discrepancy;
        if (current.len() < previous.len() + shift)
            current.resize(previous.len() + shift, T{});
        for (int i = 0; i < previous.len(); ++i)
            current[i + shift] -= factor * previous[i];
        if (2 * length <= index) {
            length = index + 1 - length;
            previous = move(saved);
            previous_discrepancy = discrepancy;
            shift = 1;
        } else {
            ++shift;
        }
    }
    nvector<T> result(length);
    for (int i = 0; i < length; ++i)
        result[i] = -current[i + 1];
    return result;
}

template <nindexed A, nindexed C>
    requires same_as<nindex_value_t<const A>, nindex_value_t<const C>>
auto nrec_nth(const A& initial, const C& recurrence, uint64_t index)
    -> nmaybe<nindex_value_t<const A>> {
    using T = nindex_value_t<const A>;
    int order = nlen(recurrence);
    if (index < uint64_t(nlen(initial)))
        return initial[int(index)];
    if (!order || nlen(initial) < order)
        return {};
    npre(order <= INT_MAX / 2);

    auto multiply = [&](const nvector<T>& left, const nvector<T>& right) {
        nvector<T> product(2 * order - 1, T{});
        for (int i = 0; i < order; ++i)
            for (int j = 0; j < order; ++j)
                product[i + j] += left[i] * right[j];
        for (int degree = 2 * order - 1; degree-- > order;)
            for (int offset = 0; offset < order; ++offset)
                product[degree - offset - 1] += product[degree] * recurrence[offset];
        product.resize(order);
        return product;
    };

    nvector<T> coefficient(order, T{}), power(order, T{});
    coefficient[0] = T{1};
    if (order == 1)
        power[0] = recurrence[0];
    else
        power[1] = T{1};
    while (index) {
        if (index & 1)
            coefficient = multiply(coefficient, power);
        index >>= 1;
        if (index)
            power = multiply(power, power);
    }
    T result{};
    for (int i = 0; i < order; ++i)
        result += coefficient[i] * initial[i];
    return result;
}

template <nindexed A, nindexed C>
    requires same_as<nindex_value_t<const A>, nindex_value_t<const C>>
auto nrec_nth(const A& initial, const C& recurrence, uint64_t index,
              nindex_value_t<const A> fallback) {
    auto result = nrec_nth(initial, recurrence, index);
    return result ? result.val() : move(fallback);
}
