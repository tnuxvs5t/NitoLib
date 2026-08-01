template <uint64_t Modulus>
    requires(Modulus > 1)
class nmodint {
    uint64_t value_ = 0;

    template <integral I> static constexpr uint64_t normalize(I value) {
        if constexpr (is_signed_v<I>) {
            __int128_t remainder = __int128_t(value) % __int128_t(Modulus);
            if (remainder < 0)
                remainder += Modulus;
            return uint64_t(remainder);
        } else {
            return uint64_t(__uint128_t(value) % Modulus);
        }
    }

  public:
    static constexpr uint64_t mod() noexcept { return Modulus; }
    constexpr nmodint() = default;
    template <integral I> constexpr nmodint(I value) : value_(normalize(value)) {}

    constexpr uint64_t val() const noexcept { return value_; }

    constexpr nmodint& operator+=(nmodint other) {
        __uint128_t sum = __uint128_t(value_) + other.value_;
        value_ = uint64_t(sum >= Modulus ? sum - Modulus : sum);
        return *this;
    }
    constexpr nmodint& operator-=(nmodint other) {
        value_ = value_ >= other.value_ ? value_ - other.value_ : Modulus - (other.value_ - value_);
        return *this;
    }
    constexpr nmodint& operator*=(nmodint other) {
        value_ = nmulmod(value_, other.value_, Modulus);
        return *this;
    }

    constexpr nmodint pow(uint64_t exponent) const { return nmodint::raw(npowmod(value_, exponent, Modulus)); }

    constexpr nmaybe<nmodint> inverse() const {
        if (!value_)
            return {};
        __int128_t old_remainder = Modulus, remainder = value_;
        __int128_t old_coefficient = 0, coefficient = 1;
        while (remainder) {
            __int128_t quotient = old_remainder / remainder;
            tie(old_remainder, remainder) = pair{remainder, old_remainder - quotient * remainder};
            tie(old_coefficient, coefficient) =
                pair{coefficient, old_coefficient - quotient * coefficient};
        }
        if (old_remainder != 1)
            return {};
        old_coefficient %= __int128_t(Modulus);
        if (old_coefficient < 0)
            old_coefficient += Modulus;
        return raw(uint64_t(old_coefficient));
    }

    constexpr nmodint& operator/=(nmodint other) {
        auto inverse = other.inverse();
        npre(inverse.ok());
        return *this *= inverse.val();
    }

    constexpr nmodint operator+() const { return *this; }
    constexpr nmodint operator-() const { return value_ ? raw(Modulus - value_) : *this; }

    friend constexpr nmodint operator+(nmodint a, nmodint b) { return a += b; }
    friend constexpr nmodint operator-(nmodint a, nmodint b) { return a -= b; }
    friend constexpr nmodint operator*(nmodint a, nmodint b) { return a *= b; }
    friend constexpr nmodint operator/(nmodint a, nmodint b) { return a /= b; }
    friend constexpr bool operator==(nmodint, nmodint) = default;

    static constexpr nmodint raw(uint64_t value) {
        npre(value < Modulus);
        nmodint result;
        result.value_ = value;
        return result;
    }
};

template <uint64_t Modulus> inline constexpr bool nadd_group<nmodint<Modulus>> = true;

template <class Mint> class ncomb {
    nvector<Mint> factorial_, inverse_factorial_;

    static int checked_extent(int n) {
        npre(0 <= n && n < INT_MAX);
        return n + 1;
    }

  public:
    explicit ncomb(int n = 0) : factorial_(checked_extent(n)), inverse_factorial_(factorial_.len()) {
        factorial_[0] = Mint(1);
        for (int i = 1; i <= n; ++i)
            factorial_[i] = factorial_[i - 1] * Mint(i);
        auto inverse = factorial_[n].inverse();
        npre(inverse.ok());
        inverse_factorial_[n] = inverse.val();
        for (int i = n; i; --i)
            inverse_factorial_[i - 1] = inverse_factorial_[i] * Mint(i);
    }

    int len() const noexcept { return factorial_.len() - 1; }
    Mint factorial(int n) const {
        npre(0 <= n && n <= len());
        return factorial_[n];
    }
    Mint choose(int n, int k) const {
        npre(0 <= n && n <= len());
        if (k < 0 || n < k)
            return Mint{};
        return factorial_[n] * inverse_factorial_[k] * inverse_factorial_[n - k];
    }
    Mint permute(int n, int k) const {
        npre(0 <= n && n <= len());
        if (k < 0 || n < k)
            return Mint{};
        return factorial_[n] * inverse_factorial_[n - k];
    }
};
