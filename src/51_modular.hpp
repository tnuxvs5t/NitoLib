template <uint64_t Modulus>
    requires(Modulus > 0)
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
    constexpr explicit operator uint64_t() const noexcept { return value_; }

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

    constexpr nmaybe<nmodint> tryinv() const { return inverse(); }
    constexpr nmodint inv() const {
        auto result = inverse();
        npre(result.ok());
        return result.val();
    }
    constexpr nmodint inv(nmodint fallback) const {
        auto result = inverse();
        return result ? result.val() : move(fallback);
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

    constexpr nmodint& operator++() { return *this += nmodint(1); }
    constexpr nmodint operator++(int) {
        nmodint copy = *this;
        ++*this;
        return copy;
    }
    constexpr nmodint& operator--() { return *this -= nmodint(1); }
    constexpr nmodint operator--(int) {
        nmodint copy = *this;
        --*this;
        return copy;
    }

    friend ostream& operator<<(ostream& output, nmodint value) { return output << value.value_; }
    friend istream& operator>>(istream& input, nmodint& value) {
        long long raw_value;
        input >> raw_value;
        value = nmodint(raw_value);
        return input;
    }

    static constexpr nmodint raw(uint64_t value) {
        npre(value < Modulus);
        nmodint result;
        result.value_ = value;
        return result;
    }
};

template <int Tag = 0> class nmod_dynamic {
    static inline uint64_t modulus_ = 1;
    uint64_t value_ = 0;

    template <integral I> static uint64_t normalize(I value) {
        uint64_t modulus = mod();
        if constexpr (is_signed_v<I>) {
            __int128_t remainder = __int128_t(value) % __int128_t(modulus);
            if (remainder < 0)
                remainder += modulus;
            return uint64_t(remainder);
        } else {
            return uint64_t(__uint128_t(value) % modulus);
        }
    }

  public:
    static uint64_t mod() noexcept { return modulus_; }
    static void setmod(uint64_t modulus) {
        npre(0 < modulus && modulus <= uint64_t(numeric_limits<int64_t>::max()));
        modulus_ = modulus;
    }
    static nmod_dynamic raw(uint64_t value) {
        npre(value < mod());
        nmod_dynamic result;
        result.value_ = value;
        return result;
    }

    nmod_dynamic() = default;
    template <integral I> nmod_dynamic(I value) : value_(normalize(value)) {}

    uint64_t val() const noexcept { return value_; }
    explicit operator uint64_t() const noexcept { return value_; }

    nmod_dynamic& operator+=(nmod_dynamic other) {
        __uint128_t sum = __uint128_t(value_) + other.value_;
        value_ = uint64_t(sum >= mod() ? sum - mod() : sum);
        return *this;
    }
    nmod_dynamic& operator-=(nmod_dynamic other) {
        value_ = value_ >= other.value_ ? value_ - other.value_ : mod() - (other.value_ - value_);
        return *this;
    }
    nmod_dynamic& operator*=(nmod_dynamic other) {
        value_ = nmulmod(value_, other.value_, mod());
        return *this;
    }
    nmod_dynamic pow(uint64_t exponent) const {
        return raw(npowmod(value_, exponent, mod()));
    }
    nmaybe<nmod_dynamic> inverse() const {
        if (!value_)
            return {};
        auto bezout = nextgcd(mod(), value_);
        if (bezout.gcd != 1)
            return {};
        __int128_t coefficient = bezout.y % __int128_t(mod());
        if (coefficient < 0)
            coefficient += mod();
        return raw(uint64_t(coefficient));
    }
    nmaybe<nmod_dynamic> tryinv() const { return inverse(); }
    nmod_dynamic inv() const {
        auto result = inverse();
        npre(result.ok());
        return result.val();
    }
    nmod_dynamic inv(nmod_dynamic fallback) const {
        auto result = inverse();
        return result ? result.val() : move(fallback);
    }
    nmod_dynamic& operator/=(nmod_dynamic other) { return *this *= other.inv(); }

    nmod_dynamic operator+() const { return *this; }
    nmod_dynamic operator-() const { return value_ ? raw(mod() - value_) : *this; }
    friend nmod_dynamic operator+(nmod_dynamic left, nmod_dynamic right) { return left += right; }
    friend nmod_dynamic operator-(nmod_dynamic left, nmod_dynamic right) { return left -= right; }
    friend nmod_dynamic operator*(nmod_dynamic left, nmod_dynamic right) { return left *= right; }
    friend nmod_dynamic operator/(nmod_dynamic left, nmod_dynamic right) { return left /= right; }
    friend bool operator==(nmod_dynamic, nmod_dynamic) = default;

    nmod_dynamic& operator++() { return *this += nmod_dynamic(1); }
    nmod_dynamic operator++(int) {
        nmod_dynamic copy = *this;
        ++*this;
        return copy;
    }
    nmod_dynamic& operator--() { return *this -= nmod_dynamic(1); }
    nmod_dynamic operator--(int) {
        nmod_dynamic copy = *this;
        --*this;
        return copy;
    }

    friend ostream& operator<<(ostream& output, nmod_dynamic value) { return output << value.value_; }
    friend istream& operator>>(istream& input, nmod_dynamic& value) {
        long long raw_value;
        input >> raw_value;
        value = nmod_dynamic(raw_value);
        return input;
    }
};

template <uint64_t Modulus> using nmod_static = nmodint<Modulus>;
template <uint64_t Modulus> using nmod = nmodint<Modulus>;
template <int Tag = 0> using ndmod = nmod_dynamic<Tag>;

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
