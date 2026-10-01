#pragma once
#include "core.hpp"

// Exact bounded rational. I is signed and at most 64 bits; every stored value is
// reduced, with a positive denominator. Arithmetic forms its exact signed-128
// result before reduction, so cancellation is not lost to an intermediate narrow
// overflow. A canonical result outside I throws; failed compound assignment keeps
// its left operand unchanged.
template <class I = long long>
struct nfrac {
    static_assert(numeric_limits<I>::is_integer && numeric_limits<I>::is_signed &&
                  numeric_limits<I>::digits <= 63);
    using value_type = I;

    constexpr nfrac() = default;

    template <class A, class B = I>
        requires (numeric_limits<A>::is_integer && numeric_limits<A>::digits <= 64 &&
                  numeric_limits<B>::is_integer && numeric_limits<B>::digits <= 64)
    constexpr nfrac(A numerator, B denominator = 1) {
        *this = reduced(wide(numerator), wide(denominator));
    }

    constexpr I numerator() const { return num; }
    constexpr I denominator() const { return den; }
    constexpr explicit operator long double() const {
        return static_cast<long double>(num) / den;
    }

    constexpr nfrac inv() const { return reduced(wide(den), wide(num)); }
    constexpr nfrac operator+() const { return *this; }
    constexpr nfrac operator-() const { return reduced(-wide(num), wide(den)); }

    constexpr nfrac& operator+=(nfrac other) {
        return *this = reduced(wide(num) * other.den + wide(other.num) * den,
                               wide(den) * other.den);
    }
    constexpr nfrac& operator-=(nfrac other) {
        return *this = reduced(wide(num) * other.den - wide(other.num) * den,
                               wide(den) * other.den);
    }
    constexpr nfrac& operator*=(nfrac other) {
        return *this = reduced(wide(num) * other.num, wide(den) * other.den);
    }
    constexpr nfrac& operator/=(nfrac other) {
        return *this = reduced(wide(num) * other.den, wide(den) * other.num);
    }

    friend constexpr nfrac operator+(nfrac a, nfrac b) { return a += b; }
    friend constexpr nfrac operator-(nfrac a, nfrac b) { return a -= b; }
    friend constexpr nfrac operator*(nfrac a, nfrac b) { return a *= b; }
    friend constexpr nfrac operator/(nfrac a, nfrac b) { return a /= b; }
    friend constexpr bool operator==(nfrac, nfrac) = default;
    friend constexpr auto operator<=>(nfrac left, nfrac right) {
        return wide(left.num) * right.den <=> wide(right.num) * left.den;
    }

    friend ostream& operator<<(ostream& out, nfrac value) {
        return out << static_cast<long long>(value.num) << '/'
                   << static_cast<long long>(value.den);
    }

private:
    using wide = __int128_t;
    I num = 0, den = 1;

    static constexpr nfrac reduced(wide numerator, wide denominator) {
        if (!denominator) throw domain_error("nfrac: zero denominator or division by zero");
        if (denominator < 0) numerator = -numerator, denominator = -denominator;

        wide a = numerator < 0 ? -numerator : numerator, b = denominator;
        while (b) {
            wide next = a % b;
            a = b;
            b = next;
        }
        numerator /= a;
        denominator /= a;
        if (numerator < numeric_limits<I>::min() ||
            numerator > numeric_limits<I>::max() ||
            denominator > numeric_limits<I>::max())
            throw overflow_error("nfrac: reduced numerator or denominator out of range");

        nfrac result;
        result.num = static_cast<I>(numerator);
        result.den = static_cast<I>(denominator);
        return result;
    }
};
