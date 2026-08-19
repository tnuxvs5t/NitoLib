#pragma once
#include "core.hpp"

/*
Mathematical floor/ceiling division for one built-in integer type I.  The divisor must be
nonzero, and the ordinary C++ quotient a / b itself must be representable (in particular,
the signed minimum divided by -1 is outside this contract).  Signed and unsigned I are
handled separately so the sign of a negative numerator is never lost to an unsigned
conversion.
*/
template <class I>
constexpr I ndiv_floor(I a, I b) {
    I quotient = a / b, remainder = a % b;
    if constexpr (numeric_limits<I>::is_signed) {
        if (remainder != 0 && ((remainder < 0) != (b < 0))) --quotient;
    }
    return quotient;
}

template <class I>
constexpr I ndiv_ceil(I a, I b) {
    I quotient = a / b, remainder = a % b;
    if constexpr (numeric_limits<I>::is_signed) {
        if (remainder != 0 && ((remainder < 0) == (b < 0))) ++quotient;
    } else if (remainder != 0) {
        ++quotient;
    }
    return quotient;
}

/* exponent is nonnegative; one is the multiplication identity. */
template <class T, class E, class M>
constexpr T npow(T base, E exponent, T one, M multiply) {
    while (exponent) {
        if (exponent & 1) one = invoke(multiply, move(one), base);
        exponent >>= 1;
        if (exponent) base = invoke(multiply, base, base);
    }
    return one;
}

template <class T, class E>
constexpr T npow(T base, E exponent) {
    return npow(move(base), exponent, T(1), multiplies<>{});
}

struct negcd_result {
    long long gcd, x, y;
};

constexpr negcd_result next_gcd(long long a, long long b) {
    long long old_x = 1, x = 0, old_y = 0, y = 1;
    while (b) {
        long long quotient = a / b;
        a -= quotient * b;
        swap(a, b);
        old_x -= quotient * x;
        swap(old_x, x);
        old_y -= quotient * y;
        swap(old_y, y);
    }
    if (a < 0) a = -a, old_x = -old_x, old_y = -old_y;
    return {a, old_x, old_y};
}

/* modulus is positive; value % modulus converts to long long; nullopt means no inverse. */
template <class I>
constexpr optional<long long> ninv_mod(I value, long long modulus) {
    long long reduced_value = static_cast<long long>(value % modulus);
    if (reduced_value < 0) reduced_value += modulus;
    auto [gcd, x, y] = next_gcd(reduced_value, modulus);
    (void)y;
    if (gcd != 1) return nullopt;
    x %= modulus;
    if (x < 0) x += modulus;
    return x;
}

/* Forward declarations let CRT reuse the same standard-only modular kernels. */
template <class I>
constexpr long long nmod_norm(I value, long long modulus);

constexpr long long nmod_add_canonical(long long left, long long right, long long modulus);
constexpr long long nmod_mul_canonical(long long left, long long right, long long modulus);

/* Positive moduli; residues may be wider; the final lcm must fit long long. */
template <class A, class B>
constexpr optional<pair<long long, long long>>
ncrt(A a, long long modulus_a, B b, long long modulus_b) {
    long long residue_a = static_cast<long long>(a % modulus_a);
    long long residue_b = static_cast<long long>(b % modulus_b);
    if (residue_a < 0) residue_a += modulus_a;
    if (residue_b < 0) residue_b += modulus_b;
    auto [gcd, x, y] = next_gcd(modulus_a, modulus_b);
    (void)y;
    long long difference = residue_b - residue_a;
    if (difference % gcd) return nullopt;
    long long reduced = modulus_b / gcd;
    long long step = nmod_mul_canonical(nmod_norm(difference / gcd, reduced),
                                        nmod_norm(x, reduced), reduced);
    long long modulus = modulus_a / gcd * modulus_b;
    long long value = nmod_add_canonical(
        nmod_mul_canonical(nmod_norm(modulus_a, modulus), nmod_norm(step, modulus), modulus),
        nmod_norm(residue_a, modulus), modulus);
    return pair{value, modulus};
}

/*
Runtime-modulus helpers.  The modulus is positive and fits signed long long; the
canonical residue type is long long, not nidx_t.  The two-argument kernels require
their operands to already lie in [0,modulus).  Wider-operand overloads normalize
before entering the canonical kernels.  Addition uses standard unsigned arithmetic;
signed __int128_t protects multiplication when the modulus approaches LLONG_MAX.
*/
template <class I>
constexpr long long nmod_norm(I value, long long modulus) {
    auto residue = value % modulus;
    long long result = static_cast<long long>(residue);
    if (result < 0) result += modulus;
    return result;
}

constexpr long long nmod_add_canonical(long long left, long long right, long long modulus) {
    uint64_t result = static_cast<uint64_t>(left) + static_cast<uint64_t>(right);
    if (result >= static_cast<uint64_t>(modulus)) result -= static_cast<uint64_t>(modulus);
    return static_cast<long long>(result);
}

constexpr long long nmod_sub_canonical(long long left, long long right, long long modulus) {
    return left >= right ? left - right : modulus - (right - left);
}

constexpr long long nmod_mul_canonical(long long left, long long right, long long modulus) {
    return static_cast<long long>(__int128_t(left) * right % modulus);
}

constexpr long long nmod_neg_canonical(long long value, long long modulus) {
    return value ? modulus - value : 0;
}

template <class A, class B>
constexpr long long nmod_add(A left, B right, long long modulus) {
    return nmod_add_canonical(nmod_norm(left, modulus), nmod_norm(right, modulus), modulus);
}

template <class A, class B>
constexpr long long nmod_sub(A left, B right, long long modulus) {
    return nmod_sub_canonical(nmod_norm(left, modulus), nmod_norm(right, modulus), modulus);
}

template <class A, class B>
constexpr long long nmod_mul(A left, B right, long long modulus) {
    return nmod_mul_canonical(nmod_norm(left, modulus), nmod_norm(right, modulus), modulus);
}

template <class I>
constexpr long long nmod_neg(I value, long long modulus) {
    return nmod_neg_canonical(nmod_norm(value, modulus), modulus);
}

/*
MOD is a positive integral constant representable by signed long long.  The
auto non-type parameter deliberately avoids imposing nidx_t on the modulus; for
example, nmodint<4000000007LL> is a valid type.  Source values are reduced before
narrowing, and exponents are nonnegative.  Division requires an invertible divisor.
*/
template <auto MOD>
struct nmodint {
    static_assert(numeric_limits<decltype(MOD)>::is_integer);
    static_assert(MOD > 0 && MOD <= LLONG_MAX);
    using value_type = long long;
    value_type value = 0;
    static constexpr value_type mod() { return static_cast<value_type>(MOD); }
    constexpr nmodint() = default;
    template <class I>
    constexpr nmodint(I x) : value(nmod_norm(x, mod())) {}
    constexpr explicit operator value_type() const { return value; }
    constexpr nmodint& operator+=(nmodint other) {
        value = nmod_add_canonical(value, other.value, mod());
        return *this;
    }
    constexpr nmodint& operator-=(nmodint other) {
        value = nmod_sub_canonical(value, other.value, mod());
        return *this;
    }
    constexpr nmodint& operator*=(nmodint other) {
        value = nmod_mul_canonical(value, other.value, mod());
        return *this;
    }
    template <class E>
    constexpr nmodint pow(E exponent) const { return npow(*this, exponent); }
    constexpr nmodint inv() const { return nmodint(*ninv_mod(value, mod())); }
    constexpr nmodint& operator/=(nmodint other) { return *this *= other.inv(); }
    friend constexpr nmodint operator+(nmodint a, nmodint b) { return a += b; }
    friend constexpr nmodint operator-(nmodint a, nmodint b) { return a -= b; }
    friend constexpr nmodint operator*(nmodint a, nmodint b) { return a *= b; }
    friend constexpr nmodint operator/(nmodint a, nmodint b) { return a /= b; }
    friend constexpr nmodint operator-(nmodint a) { return nmodint(nmod_neg_canonical(a.value, mod())); }
    friend constexpr auto operator<=>(nmodint, nmodint) = default;
    friend ostream& operator<<(ostream& out, nmodint x) { return out << x.value; }
    friend istream& operator>>(istream& in, nmodint& x) {
        string token;
        if (!(in >> token)) return in;
        nidx_t at = 0;
        bool negative = false;
        if (token[at] == '+' || token[at] == '-') {
            negative = token[at] == '-';
            if (++at == nidx_t(token.size())) return in.setstate(ios::failbit), in;
        }
        value_type residue = 0;
        for (; at < nidx_t(token.size()); ++at) {
            nidx_t digit = token[at] - '0';
            if (digit < 0 || digit > 9) return in.setstate(ios::failbit), in;
            residue = nmod_add_canonical(nmod_mul_canonical(residue, 10, mod()), digit, mod());
        }
        x.value = negative ? nmod_neg_canonical(residue, mod()) : residue;
        return in;
    }
};

/* Factorial division requires every denominator in [1,n] to be invertible. */
template <class M>
struct ncomb {
    vector<M> factorial{M(1)}, inverse_factorial{M(1)};
    explicit ncomb(nidx_t n = 0) { extend(n); }
    void extend(nidx_t n) {
        nidx_t old = nidx_t(factorial.size()) - 1;
        if (n <= old) return;
        factorial.resize(n + 1);
        for (nidx_t i = old + 1; i <= n; ++i) factorial[i] = factorial[i - 1] * M(i);
        inverse_factorial.resize(n + 1);
        inverse_factorial[n] = factorial[n].inv();
        for (nidx_t i = n; i > old; --i) inverse_factorial[i - 1] = inverse_factorial[i] * M(i);
    }
    M permutation(nidx_t n, nidx_t k) const { return factorial[n] * inverse_factorial[n - k]; }
    M choose(nidx_t n, nidx_t k) const {
        return k < 0 || k > n ? M{} : factorial[n] * inverse_factorial[k] * inverse_factorial[n - k];
    }
};

struct nsieve {
    vector<nidx_t> least, primes;
    explicit nsieve(nidx_t n = 0) : least(n + 1) {
        for (nidx_t value = 2; value <= n; ++value) {
            if (!least[value]) least[value] = value, primes.push_back(value);
            for (nidx_t prime : primes) {
                if (prime > least[value] || 1LL * prime * value > n) break;
                least[prime * value] = prime;
            }
        }
    }
    bool prime(nidx_t value) const { return value >= 2 && least[value] == value; }
    vector<pair<nidx_t, nidx_t>> factor(nidx_t value) const {
        vector<pair<nidx_t, nidx_t>> result;
        while (value > 1) {
            nidx_t prime = least[value], exponent = 0;
            do value /= prime, ++exponent; while (value > 1 && least[value] == prime);
            result.emplace_back(prime, exponent);
        }
        return result;
    }
    nidx_t phi(nidx_t value) const {
        nidx_t result = value;
        for (auto [prime, exponent] : factor(value)) result -= result / prime, (void)exponent;
        return result;
    }
};
