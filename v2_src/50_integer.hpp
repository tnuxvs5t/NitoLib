template <integral T> constexpr make_unsigned_t<T> nmag(T value) {
    using U = make_unsigned_t<T>;
    U encoded = U(value);
    if constexpr (is_signed_v<T>)
        return value < 0 ? U{} - encoded : encoded;
    else
        return encoded;
}

template <integral T> constexpr make_unsigned_t<T> nabs(T value) { return nmag(value); }

template <integral T> constexpr make_unsigned_t<T> ngcd(T a, T b) {
    using U = make_unsigned_t<T>;
    U x = nmag(a), y = nmag(b);
    while (y) {
        U remainder = x % y;
        x = y;
        y = remainder;
    }
    return x;
}

template <integral T> constexpr make_unsigned_t<T> nlcm(T a, T b) {
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

template <signed_integral T> constexpr T nmod(T value, T modulus) {
    npre(modulus > 0);
    T remainder = value % modulus;
    return remainder < 0 ? remainder + modulus : remainder;
}

template <integral T> struct nextgcd_result {
    make_unsigned_t<T> gcd;
    __int128_t x, y;
};

template <integral T> constexpr nextgcd_result<T> nextgcd(T a, T b) {
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
