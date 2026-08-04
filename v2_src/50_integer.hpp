template <class T>
concept ninteger = integral<T> && (!same_as<remove_cv_t<T>, bool>);

template <ninteger T> constexpr make_unsigned_t<T> nmag(T value) {
    using U = make_unsigned_t<T>;
    U encoded = U(value);
    if constexpr (is_signed_v<T>)
        return value < 0 ? U{} - encoded : encoded;
    else
        return encoded;
}

template <ninteger T> constexpr make_unsigned_t<T> nabs(T value) { return nmag(value); }

template <ninteger T> constexpr make_unsigned_t<T> ngcd(T a, T b) {
    using U = make_unsigned_t<T>;
    U x = nmag(a), y = nmag(b);
    while (y) {
        U remainder = x % y;
        x = y;
        y = remainder;
    }
    return x;
}

template <ninteger T> constexpr make_unsigned_t<T> nlcm(T a, T b) {
    using U = make_unsigned_t<T>;
    U x = nmag(a), y = nmag(b);
    if (!x || !y)
        return 0;
    U divisor = ngcd(a, b);
    npre(x / divisor <= numeric_limits<U>::max() / y);
    return x / divisor * y;
}

template <signed_integral T> constexpr T nfloor_div(T a, T b) {
    npre(b != 0);
    npre(!(a == numeric_limits<T>::lowest() && b == T(-1)));
    T quotient = a / b, remainder = a % b;
    if (remainder && ((remainder < 0) != (b < 0)))
        --quotient;
    return quotient;
}

template <signed_integral T> constexpr T nceil_div(T a, T b) {
    npre(b != 0);
    npre(!(a == numeric_limits<T>::lowest() && b == T(-1)));
    T quotient = a / b, remainder = a % b;
    if (remainder && ((remainder < 0) == (b < 0)))
        ++quotient;
    return quotient;
}

template <signed_integral T> constexpr T nmodulo(T value, T modulus) {
    npre(modulus > 0);
    T remainder = value % modulus;
    return remainder < 0 ? remainder + modulus : remainder;
}

template <ninteger T>
    requires(sizeof(T) <= sizeof(uint64_t))
struct nextgcd_result {
    make_unsigned_t<T> gcd;
    __int128_t x, y;
};

template <ninteger T>
    requires(sizeof(T) <= sizeof(uint64_t))
constexpr nextgcd_result<T> nextgcd(T a, T b) {
    using U = make_unsigned_t<T>;
    U old_remainder = nmag(a), remainder = nmag(b);
    __int128_t old_x = 1, x = 0, old_y = 0, y = 1;
    while (remainder) {
        U quotient = old_remainder / remainder;
        U next_remainder = old_remainder % remainder;
        old_remainder = remainder;
        remainder = next_remainder;
        __int128_t next_x = old_x - __int128_t(quotient) * x;
        __int128_t next_y = old_y - __int128_t(quotient) * y;
        old_x = x;
        x = next_x;
        old_y = y;
        y = next_y;
    }
    if constexpr (is_signed_v<T>) {
        if (a < 0)
            old_x = -old_x;
        if (b < 0)
            old_y = -old_y;
    }
    return {old_remainder, old_x, old_y};
}

constexpr uint64_t nmulmod(uint64_t a, uint64_t b, uint64_t modulus) {
    npre(modulus > 0);
    return uint64_t(__uint128_t(a) * b % modulus);
}

constexpr uint64_t npowmod(uint64_t base, uint64_t exponent, uint64_t modulus) {
    npre(modulus > 0);
    uint64_t result = 1 % modulus;
    base %= modulus;
    while (exponent) {
        if (exponent & 1)
            result = nmulmod(result, base, modulus);
        exponent >>= 1;
        if (exponent)
            base = nmulmod(base, base, modulus);
    }
    return result;
}

constexpr bool nisprime(uint64_t value) {
    if (value < 2)
        return false;
    for (uint64_t prime : {2ULL, 3ULL, 5ULL, 7ULL, 11ULL, 13ULL, 17ULL, 19ULL, 23ULL, 29ULL, 31ULL, 37ULL}) {
        if (value % prime == 0)
            return value == prime;
    }
    uint64_t odd = value - 1;
    int shifts = countr_zero(odd);
    odd >>= shifts;
    for (uint64_t base : {2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL, 9780504ULL, 1795265022ULL}) {
        if (base % value == 0)
            continue;
        uint64_t witness = npowmod(base, odd, value);
        if (witness == 1 || witness == value - 1)
            continue;
        bool composite = true;
        for (int round = 1; round < shifts; ++round) {
            witness = nmulmod(witness, witness, value);
            if (witness == value - 1) {
                composite = false;
                break;
            }
        }
        if (composite)
            return false;
    }
    return true;
}

inline nvector<int> nprimes(int limit) {
    npre(0 <= limit && limit < INT_MAX);
    nvector<int> primes, least(limit + 1, 0);
    for (int value = 2; value <= limit; ++value) {
        if (!least[value]) {
            least[value] = value;
            primes.push(value);
        }
        for (int index = 0; index < primes.len(); ++index) {
            int prime = primes[index];
            if (prime > least[value] || 1LL * value * prime > limit)
                break;
            least[value * prime] = prime;
        }
    }
    return primes;
}

inline bool nisprime_trial(uint64_t value) {
    if (value < 2)
        return false;
    for (uint64_t divisor = 2; divisor <= value / divisor; ++divisor)
        if (value % divisor == 0)
            return false;
    return true;
}

constexpr bool nisprime_miller(uint64_t value) { return nisprime(value); }

inline uint64_t npollard(uint64_t value) {
    npre(value > 1 && !nisprime(value));
    if (!(value & 1))
        return 2;
    if (value % 3 == 0)
        return 3;

    auto advance = [value](uint64_t x, uint64_t constant) {
        return uint64_t((__uint128_t(nmulmod(x, x, value)) + constant) % value);
    };
    for (;;) {
        uint64_t current = nrng_global(uint64_t(1), value);
        uint64_t constant = nrng_global(uint64_t(1), value);
        uint64_t block = 128, power = 1, divisor = 1;
        uint64_t anchor = 0, recovery = 0;
        while (divisor == 1) {
            anchor = current;
            for (uint64_t step = 0; step < power; ++step)
                current = advance(current, constant);
            for (uint64_t offset = 0; offset < power && divisor == 1; offset += block) {
                recovery = current;
                uint64_t product = 1;
                uint64_t count = min(block, power - offset);
                for (uint64_t step = 0; step < count; ++step) {
                    current = advance(current, constant);
                    uint64_t difference = anchor > current ? anchor - current : current - anchor;
                    product = nmulmod(product, difference, value);
                }
                divisor = gcd(product, value);
            }
            if (power > numeric_limits<uint64_t>::max() / 2) {
                divisor = value;
                break;
            }
            power *= 2;
        }
        if (divisor == value) {
            do {
                recovery = advance(recovery, constant);
                uint64_t difference = anchor > recovery ? anchor - recovery : recovery - anchor;
                divisor = gcd(difference, value);
            } while (divisor == 1);
        }
        if (1 < divisor && divisor < value)
            return divisor;
    }
}

inline nvector<uint64_t> nfactor(uint64_t value) {
    nvector<uint64_t> factors, pending;
    if (value > 1)
        pending.push(value);
    while (!pending.empty()) {
        uint64_t current = pending.pop();
        if (nisprime(current)) {
            factors.push(current);
        } else {
            uint64_t divisor = npollard(current);
            pending.push(divisor);
            pending.push(current / divisor);
        }
    }
    nsort(factors);
    return factors;
}

inline nvector<uint64_t> nfactor_rho(uint64_t value) { return nfactor(value); }

template <signed_integral T = long long> class nfrac {
    using W = nwide_t<T>;
    using U = make_unsigned_t<W>;

    static U magnitude(W value) {
        U encoded = U(value);
        return value < 0 ? U{} - encoded : encoded;
    }
    static U gcd_wide(U a, U b) {
        while (b) {
            U remainder = a % b;
            a = b;
            b = remainder;
        }
        return a;
    }
    static T narrow(W value) { return ni::nchecked_integral_cast<T>(value); }
    void assign(W numerator, W denominator) {
        npre(denominator != 0);
        U divisor = gcd_wide(magnitude(numerator), magnitude(denominator));
        numerator /= W(divisor);
        denominator /= W(divisor);
        if (denominator < 0) {
            npre(numerator != numeric_limits<W>::lowest());
            numerator = -numerator;
            denominator = -denominator;
        }
        p = narrow(numerator);
        q = narrow(denominator);
    }

  public:
    T p = 0, q = 1;

    constexpr nfrac() = default;
    constexpr nfrac(T integer) : p(integer) {}
    nfrac(T numerator, T denominator) { assign(W(numerator), W(denominator)); }

    nfrac& operator+=(const nfrac& other) {
        U divisor = gcd_wide(U(q), U(other.q));
        W left_scale = W(other.q) / W(divisor);
        W right_scale = W(q) / W(divisor);
        W numerator;
        npre(!__builtin_mul_overflow(W(p), left_scale, &numerator));
        W addend;
        npre(!__builtin_mul_overflow(W(other.p), right_scale, &addend));
        npre(!__builtin_add_overflow(numerator, addend, &numerator));
        W denominator;
        npre(!__builtin_mul_overflow(right_scale, W(other.q), &denominator));
        assign(numerator, denominator);
        return *this;
    }
    nfrac& operator-=(const nfrac& other) { return *this += -other; }
    nfrac& operator*=(const nfrac& other) {
        U left_cancel = gcd_wide(magnitude(W(p)), U(other.q));
        U right_cancel = gcd_wide(magnitude(W(other.p)), U(q));
        W numerator, denominator;
        npre(!__builtin_mul_overflow(W(p) / W(left_cancel), W(other.p) / W(right_cancel),
                                     &numerator));
        npre(!__builtin_mul_overflow(W(q) / W(right_cancel), W(other.q) / W(left_cancel),
                                     &denominator));
        assign(numerator, denominator);
        return *this;
    }
    nmaybe<nfrac> trydiv(const nfrac& other) const {
        if (!other.p)
            return {};
        nfrac result = *this;
        result *= nfrac(other.q, other.p);
        return result;
    }
    nfrac& operator/=(const nfrac& other) {
        auto result = trydiv(other);
        npre(result.ok());
        return *this = move(result.val());
    }
    nfrac operator+() const { return *this; }
    nfrac operator-() const {
        npre(p != numeric_limits<T>::lowest());
        return nfrac(T(-p), q);
    }
    friend nfrac operator+(nfrac left, const nfrac& right) { return left += right; }
    friend nfrac operator-(nfrac left, const nfrac& right) { return left -= right; }
    friend nfrac operator*(nfrac left, const nfrac& right) { return left *= right; }
    friend nfrac operator/(nfrac left, const nfrac& right) { return left /= right; }
    friend bool operator==(const nfrac&, const nfrac&) = default;
    friend strong_ordering operator<=>(const nfrac& left, const nfrac& right) {
        W a = W(left.p) * right.q;
        W b = W(right.p) * left.q;
        return a < b ? strong_ordering::less
                     : a > b ? strong_ordering::greater : strong_ordering::equal;
    }
    T floor() const { return nfloor_div(p, q); }
    T ceil() const { return nceil_div(p, q); }
    explicit operator long double() const { return static_cast<long double>(p) / q; }
    friend ostream& operator<<(ostream& output, const nfrac& value) {
        return output << value.p << '/' << value.q;
    }
};

struct ncongruence {
    long long a = 0, m = 1;

    ncongruence() = default;
    ncongruence(long long residue, long long modulus) : m(modulus) {
        npre(modulus > 0);
        a = nmodulo(residue, modulus);
    }
    bool has(long long value) const { return (__int128_t(value) - a) % m == 0; }
    nmaybe<long long> at(long long index) const {
        __int128_t value = __int128_t(a) + __int128_t(index) * m;
        if (value < numeric_limits<long long>::lowest() ||
            value > numeric_limits<long long>::max())
            return {};
        return static_cast<long long>(value);
    }
    long long at(long long index, long long fallback) const {
        auto result = at(index);
        return result ? result.val() : fallback;
    }
    long long operator()(long long index) const {
        auto result = at(index);
        npre(result.ok());
        return result.val();
    }
    friend bool operator==(const ncongruence&, const ncongruence&) = default;
};

inline nmaybe<ncongruence> ncrt(ncongruence left, ncongruence right) {
    auto bezout = nextgcd(left.m, right.m);
    __int128_t difference = __int128_t(right.a) - left.a;
    if (difference % bezout.gcd)
        return {};
    __int128_t modulus = __int128_t(left.m / bezout.gcd) * right.m;
    if (modulus > numeric_limits<long long>::max())
        return {};
    __int128_t quotient_modulus = right.m / bezout.gcd;
    __int128_t multiplier = difference / bezout.gcd * bezout.x % quotient_modulus;
    __int128_t residue = (__int128_t(left.a) + __int128_t(left.m) * multiplier) % modulus;
    if (residue < 0)
        residue += modulus;
    return ncongruence(static_cast<long long>(residue), static_cast<long long>(modulus));
}

inline ncongruence ncrt(ncongruence left, ncongruence right, ncongruence fallback) {
    auto result = ncrt(left, right);
    return result ? result.val() : move(fallback);
}

class nprime_table {
    static int checked_limit(int limit) {
        npre(0 <= limit && limit < INT_MAX);
        return limit;
    }

  public:
    int n = 0;
    nvector<int> p, lp, phi, mu;

    nprime_table() = default;
    explicit nprime_table(int limit)
        : n(checked_limit(limit)), lp(n + 1, 0), phi(n + 1, 0), mu(n + 1, 0) {
        if (limit >= 1)
            phi[1] = mu[1] = 1;
        for (int value = 2; value <= limit; ++value) {
            if (!lp[value]) {
                lp[value] = value;
                p.push(value);
                phi[value] = value - 1;
                mu[value] = -1;
            }
            for (int index = 0; index < p.len(); ++index) {
                int prime = p[index];
                if (prime > lp[value] || 1LL * value * prime > limit)
                    break;
                int product = value * prime;
                lp[product] = prime;
                if (prime == lp[value]) {
                    phi[product] = phi[value] * prime;
                    mu[product] = 0;
                } else {
                    phi[product] = phi[value] * (prime - 1);
                    mu[product] = -mu[value];
                }
            }
        }
    }
    bool isprime(int value) const { return 2 <= value && value <= n && lp[value] == value; }
    nvector<pair<int, int>> factor(int value) const {
        npre(0 < value && value <= n);
        nvector<pair<int, int>> result;
        while (value > 1) {
            int prime = lp[value], exponent = 0;
            do {
                value /= prime;
                ++exponent;
            } while (value > 1 && lp[value] == prime);
            result.push(pair<int, int>{prime, exponent});
        }
        return result;
    }
    nvector<int> divisors(int value) const {
        nvector<int> result{1};
        nfor(factor, this->factor(value)) {
            int previous = result.len(), power = 1;
            for (int exponent = 1; exponent <= factor.second; ++exponent) {
                power *= factor.first;
                for (int index = 0; index < previous; ++index)
                    result.push(result[index] * power);
            }
        }
        nsort(result);
        return result;
    }
};
