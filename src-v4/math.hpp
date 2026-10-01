#pragma once
#include "core.hpp"

template <class I>
constexpr I ndiv_floor(I a, I b) {
    I q = a / b, r = a % b;
    if constexpr (numeric_limits<I>::is_signed)
        if (r && ((r < 0) != (b < 0))) --q;
    return q;
}

template <class I>
constexpr I ndiv_ceil(I a, I b) {
    I q = a / b, r = a % b;
    if constexpr (numeric_limits<I>::is_signed) {
        if (r && ((r < 0) == (b < 0))) ++q;
    } else if (r) {
        ++q;
    }
    return q;
}

constexpr __int128_t nfloor_sum(long long n, long long modulus,
                                 long long a, long long b) {
    __int128_t count = n, mod = modulus, slope = a, offset = b;
    __int128_t answer = ndiv_floor(slope, mod) * count * (count - 1) / 2 +
                        ndiv_floor(offset, mod) * count;
    slope %= mod;
    offset %= mod;
    if (slope < 0) slope += mod;
    if (offset < 0) offset += mod;
    while (true) {
        answer += (slope / mod) * count * (count - 1) / 2 +
                  (offset / mod) * count;
        slope %= mod;
        offset %= mod;
        __int128_t height = slope * count + offset;
        if (height < mod) return answer;
        count = height / mod;
        offset = height % mod;
        swap(slope, mod);
    }
}

constexpr uint64_t nisqrt(uint64_t value) {
    uint64_t low = 0, high = uint64_t(1) << 32;
    while (high - low > 1) {
        uint64_t middle = low + (high - low) / 2;
        if (middle * middle <= value) low = middle;
        else high = middle;
    }
    return low;
}

template <class F>
void nquotient_blocks(long long n, F&& visit) {
    for (long long left = 1; left <= n;) {
        long long quotient = n / left, right = n / quotient + 1;
        invoke(visit, left, right, quotient);
        left = right;
    }
}

template <class F>
void nquotient_blocks(long long a, long long b, F&& visit) {
    for (long long left = 1; left <= min(a, b);) {
        long long qa = a / left, qb = b / left;
        long long right = min(a / qa, b / qb) + 1;
        invoke(visit, left, right, qa, qb);
        left = right;
    }
}

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

struct negcd_result { long long gcd, x, y; };

constexpr negcd_result next_gcd(long long a, long long b) {
    __int128_t left = a, right = b;
    __int128_t old_x = 1, x = 0, old_y = 0, y = 1;
    while (right) {
        __int128_t q = left / right;
        left -= q * right;
        swap(left, right);
        old_x -= q * x;
        swap(old_x, x);
        old_y -= q * y;
        swap(old_y, y);
    }
    if (left < 0) left = -left, old_x = -old_x, old_y = -old_y;
    return {static_cast<long long>(left), static_cast<long long>(old_x),
            static_cast<long long>(old_y)};
}

template <class I>
constexpr optional<long long> ninv_mod(I value, long long modulus) {
    long long reduced = static_cast<long long>(value % modulus);
    if (reduced < 0) reduced += modulus;
    auto [gcd, x, y] = next_gcd(reduced, modulus);
    (void)y;
    if (gcd != 1) return nullopt;
    x %= modulus;
    if (x < 0) x += modulus;
    return x;
}

template <class I>
constexpr long long nmod_norm(I value, long long modulus) {
    auto remainder = value % modulus;
    long long result = static_cast<long long>(remainder);
    if (result < 0) result += modulus;
    return result;
}

constexpr long long nmod_add_canonical(long long left, long long right,
                                       long long modulus) {
    uint64_t result = uint64_t(left) + uint64_t(right);
    if (result >= uint64_t(modulus)) result -= uint64_t(modulus);
    return static_cast<long long>(result);
}

constexpr long long nmod_sub_canonical(long long left, long long right,
                                       long long modulus) {
    return left >= right ? left - right : modulus - (right - left);
}

constexpr long long nmod_mul_canonical(long long left, long long right,
                                       long long modulus) {
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

template <class A, class B>
constexpr optional<pair<long long, long long>>
ncrt(A a, long long modulus_a, B b, long long modulus_b) {
    long long left = nmod_norm(a, modulus_a), right = nmod_norm(b, modulus_b);
    auto [gcd, x, y] = next_gcd(modulus_a, modulus_b);
    (void)y;
    long long difference = right - left;
    if (difference % gcd) return nullopt;
    long long reduced = modulus_b / gcd;
    long long step = nmod_mul_canonical(nmod_norm(difference / gcd, reduced),
                                        nmod_norm(x, reduced), reduced);
    long long modulus = modulus_a / gcd * modulus_b;
    long long value = nmod_add_canonical(
        nmod_mul_canonical(nmod_norm(modulus_a, modulus),
                           nmod_norm(step, modulus), modulus),
        nmod_norm(left, modulus), modulus);
    return pair{value, modulus};
}

template <auto MOD>
struct nmodint {
    static_assert(numeric_limits<decltype(MOD)>::is_integer);
    static_assert(MOD > 0 && MOD <= LLONG_MAX);
    using value_type = long long;
    value_type value = 0;

    static constexpr value_type mod() { return value_type(MOD); }
    constexpr nmodint() = default;
    template <class I> constexpr nmodint(I x) : value(nmod_norm(x, mod())) {}
    constexpr explicit operator value_type() const { return value; }

    constexpr nmodint& operator+=(nmodint other) {
        value = nmod_add_canonical(value, other.value, mod()); return *this;
    }
    constexpr nmodint& operator-=(nmodint other) {
        value = nmod_sub_canonical(value, other.value, mod()); return *this;
    }
    constexpr nmodint& operator*=(nmodint other) {
        value = nmod_mul_canonical(value, other.value, mod()); return *this;
    }
    template <class E> constexpr nmodint pow(E exponent) const {
        return npow(*this, exponent);
    }
    constexpr nmodint inv() const { return nmodint(*ninv_mod(value, mod())); }
    constexpr nmodint& operator/=(nmodint other) { return *this *= other.inv(); }

    friend constexpr nmodint operator+(nmodint a, nmodint b) { return a += b; }
    friend constexpr nmodint operator-(nmodint a, nmodint b) { return a -= b; }
    friend constexpr nmodint operator*(nmodint a, nmodint b) { return a *= b; }
    friend constexpr nmodint operator/(nmodint a, nmodint b) { return a /= b; }
    friend constexpr nmodint operator-(nmodint a) {
        return nmodint(nmod_neg_canonical(a.value, mod()));
    }
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
            residue = nmod_add_canonical(nmod_mul_canonical(residue, 10 % mod(), mod()),
                                         digit % mod(), mod());
        }
        x.value = negative ? nmod_neg_canonical(residue, mod()) : residue;
        return in;
    }
};

template <class M>
struct ncomb {
    vector<M> factorial{M(1)}, inverse_factorial{M(1)};

    explicit ncomb(nidx_t n = 0) { extend(n); }
    void extend(nidx_t n) {
        nidx_t old = nidx_t(factorial.size()) - 1;
        if (n <= old) return;
        factorial.resize(n + 1);
        for (nidx_t i = old + 1; i <= n; ++i)
            factorial[i] = factorial[i - 1] * M(i);
        inverse_factorial.resize(n + 1);
        inverse_factorial[n] = factorial[n].inv();
        for (nidx_t i = n; i > old; --i)
            inverse_factorial[i - 1] = inverse_factorial[i] * M(i);
    }
    M permutation(nidx_t n, nidx_t k) const {
        return factorial[n] * inverse_factorial[n - k];
    }
    M choose(nidx_t n, nidx_t k) const {
        return k < 0 || k > n ? M{} : factorial[n] * inverse_factorial[k] * inverse_factorial[n - k];
    }
    template <class I>
    M lucas(I n, I k) const {
        M result = 1;
        auto modulus = M::mod();
        while (n || k) {
            auto a = n % modulus, b = k % modulus;
            if (b > a) return M{};
            result *= choose(nidx_t(a), nidx_t(b));
            n /= modulus;
            k /= modulus;
        }
        return result;
    }
};

template <class M, class I>
M nchoose_small(I n, I k) {
    if (k < 0 || k > n) return M{};
    k = min(k, n - k);
    M numerator = 1, denominator = 1;
    for (I i = 0; i < k; ++i) {
        numerator *= M(n - i);
        denominator *= M(i + 1);
    }
    return numerator / denominator;
}

template <class V>
auto ninverse_batch(const V& values) {
    using M = remove_cvref_t<decltype(values[0])>;
    nidx_t n = nlen(values);
    vector<M> result(n);
    if (!n) return result;
    M product = 1;
    for (nidx_t i = 0; i < n; ++i) result[i] = product, product *= values[i];
    M inverse = M(1) / product;
    for (nidx_t i = n; i-- > 0;) {
        result[i] *= inverse;
        inverse *= values[i];
    }
    return result;
}

struct nsieve {
    vector<nidx_t> least, primes;

    explicit nsieve(nidx_t n = 0) : least(n + 1) {
        for (nidx_t value = 2; value <= n; ++value) {
            if (!least[value]) least[value] = value, primes.push_back(value);
            for (nidx_t prime : primes) {
                if (prime > least[value] || prime > n / value) break;
                least[prime * value] = prime;
            }
        }
    }
    bool prime(nidx_t value) const { return value >= 2 && least[value] == value; }
    vector<pair<nidx_t, nidx_t>> factor(nidx_t value) const {
        vector<pair<nidx_t, nidx_t>> result;
        while (value > 1) {
            nidx_t prime = least[value], exponent = 0;
            do value /= prime, ++exponent;
            while (value > 1 && least[value] == prime);
            result.emplace_back(prime, exponent);
        }
        return result;
    }
    nidx_t phi(nidx_t value) const {
        nidx_t result = value;
        while (value > 1) {
            nidx_t prime = least[value];
            result -= result / prime;
            do value /= prime; while (value > 1 && least[value] == prime);
        }
        return result;
    }
    vector<nidx_t> phi_table() const {
        vector<nidx_t> result(least.size());
        if (result.size() > 1) result[1] = 1;
        for (nidx_t value = 2; value < nidx_t(result.size()); ++value) {
            nidx_t prime = least[value], rest = value / prime;
            result[value] = result[rest] * (rest % prime ? prime - 1 : prime);
        }
        return result;
    }
    vector<int8_t> mu_table() const {
        vector<int8_t> result(least.size());
        if (result.size() > 1) result[1] = 1;
        for (nidx_t value = 2; value < nidx_t(result.size()); ++value) {
            nidx_t prime = least[value], rest = value / prime;
            result[value] = rest % prime ? int8_t(-result[rest]) : 0;
        }
        return result;
    }
};
