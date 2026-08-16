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

/* modulus is positive; nullopt means the inverse does not exist. */
constexpr optional<long long> ninv_mod(long long value, long long modulus) {
    auto [gcd, x, y] = next_gcd(value, modulus);
    (void)y;
    if (gcd != 1) return nullopt;
    x %= modulus;
    if (x < 0) x += modulus;
    return x;
}

/* Returns the least nonnegative solution and lcm modulus, or nullopt if inconsistent. */
constexpr optional<pair<long long, long long>>
ncrt(long long a, long long modulus_a, long long b, long long modulus_b) {
    auto [gcd, x, y] = next_gcd(modulus_a, modulus_b);
    (void)y;
    long long difference = b - a;
    if (difference % gcd) return nullopt;
    long long reduced = modulus_b / gcd;
    long long step = static_cast<long long>((__int128(difference / gcd) * x % reduced + reduced) % reduced);
    long long modulus = modulus_a / gcd * modulus_b;
    long long value = static_cast<long long>((__int128(modulus_a) * step + a) % modulus);
    if (value < 0) value += modulus;
    return pair{value, modulus};
}

/* MOD is positive and fits signed int.  Division requires an invertible divisor. */
template <int MOD>
struct nmodint {
    int value = 0;
    static constexpr int mod() { return MOD; }
    constexpr nmodint() = default;
    constexpr nmodint(long long x) : value(int(x % MOD)) { if (value < 0) value += MOD; }
    constexpr explicit operator int() const { return value; }
    constexpr nmodint& operator+=(nmodint other) {
        if (value >= MOD - other.value) value -= MOD - other.value;
        else value += other.value;
        return *this;
    }
    constexpr nmodint& operator-=(nmodint other) {
        value -= other.value;
        if (value < 0) value += MOD;
        return *this;
    }
    constexpr nmodint& operator*=(nmodint other) {
        value = int(1LL * value * other.value % MOD);
        return *this;
    }
    constexpr nmodint pow(long long exponent) const { return npow(*this, exponent); }
    constexpr nmodint inv() const { return nmodint(*ninv_mod(value, MOD)); }
    constexpr nmodint& operator/=(nmodint other) { return *this *= other.inv(); }
    friend constexpr nmodint operator+(nmodint a, nmodint b) { return a += b; }
    friend constexpr nmodint operator-(nmodint a, nmodint b) { return a -= b; }
    friend constexpr nmodint operator*(nmodint a, nmodint b) { return a *= b; }
    friend constexpr nmodint operator/(nmodint a, nmodint b) { return a /= b; }
    friend constexpr nmodint operator-(nmodint a) { return a.value ? nmodint(MOD - a.value) : a; }
    friend constexpr auto operator<=>(nmodint, nmodint) = default;
    friend ostream& operator<<(ostream& out, nmodint x) { return out << x.value; }
    friend istream& operator>>(istream& in, nmodint& x) {
        long long value;
        return in >> value, x = nmodint(value), in;
    }
};

/* Factorial division requires every denominator in [1,n] to be invertible. */
template <class M>
struct ncomb {
    vector<M> factorial{M(1)}, inverse_factorial{M(1)};
    explicit ncomb(int n = 0) { extend(n); }
    void extend(int n) {
        int old = int(factorial.size()) - 1;
        if (n <= old) return;
        factorial.resize(n + 1);
        for (int i = old + 1; i <= n; ++i) factorial[i] = factorial[i - 1] * M(i);
        inverse_factorial.resize(n + 1);
        inverse_factorial[n] = factorial[n].inv();
        for (int i = n; i > old; --i) inverse_factorial[i - 1] = inverse_factorial[i] * M(i);
    }
    M permutation(int n, int k) const { return factorial[n] * inverse_factorial[n - k]; }
    M choose(int n, int k) const {
        return k < 0 || k > n ? M{} : factorial[n] * inverse_factorial[k] * inverse_factorial[n - k];
    }
};

struct nsieve {
    vector<int> least, primes;
    explicit nsieve(int n = 0) : least(n + 1) {
        for (int value = 2; value <= n; ++value) {
            if (!least[value]) least[value] = value, primes.push_back(value);
            for (int prime : primes) {
                if (prime > least[value] || 1LL * prime * value > n) break;
                least[prime * value] = prime;
            }
        }
    }
    bool prime(int value) const { return value >= 2 && least[value] == value; }
    vector<pair<int, int>> factor(int value) const {
        vector<pair<int, int>> result;
        while (value > 1) {
            int prime = least[value], exponent = 0;
            do value /= prime, ++exponent; while (value > 1 && least[value] == prime);
            result.emplace_back(prime, exponent);
        }
        return result;
    }
    int phi(int value) const {
        int result = value;
        for (auto [prime, exponent] : factor(value)) result -= result / prime, (void)exponent;
        return result;
    }
};
