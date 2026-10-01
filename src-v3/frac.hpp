#pragma once
#include "core.hpp"

/*
Exact bounded rational, independent of nidx_t. I is a signed built-in integer of
at most 64 bits. Constructor inputs are signed/unsigned integers of at most 64
bits; they are reduced BEFORE narrowing to I. No floating or 128-bit input conversion.
The representation is always coprime with positive denominator; zero is 0/1.

Every arithmetic operation reduces its exact signed-128 intermediate before
checking I's range. Two cross products and their sum/difference fit signed 128
bits because stored denominators are positive signed-64 values. Thus representable
results are not rejected merely because an unreduced product exceeds I.
Unrepresentable canonical results throw overflow_error; zero denominators/divisors
throw domain_error. Compound assignments leave the target unchanged on failure.
Arithmetic/construction take O(log M) Euclidean steps for intermediate magnitude M;
comparison takes O(1) wide operations. Storage and auxiliary space are O(1).
*/
template <class I = long long>
struct nfrac {
    static_assert(numeric_limits<I>::is_integer && numeric_limits<I>::is_signed
                  && numeric_limits<I>::digits <= 63);
    using value_type = I;

    constexpr nfrac() = default;
    template <class A, class B = I>
        requires (numeric_limits<A>::is_integer && numeric_limits<A>::digits <= 64
                  && numeric_limits<B>::is_integer && numeric_limits<B>::digits <= 64)
    constexpr nfrac(A numerator, B denominator = 1) {
        *this = reduced(numerator, denominator);
    }

    constexpr I numerator() const { return numerator_; }
    constexpr I denominator() const { return denominator_; }
    constexpr explicit operator long double() const {
        return static_cast<long double>(numerator_) / denominator_;
    }
    constexpr nfrac inv() const { return reduced(denominator_, numerator_); }
    constexpr nfrac operator+() const { return *this; }
    constexpr nfrac operator-() const { return reduced(-wide(numerator_), denominator_); }
    constexpr nfrac& operator+=(nfrac other) {
        return *this = reduced(wide(numerator_) * other.denominator_
                               + wide(other.numerator_) * denominator_,
                               wide(denominator_) * other.denominator_);
    }
    constexpr nfrac& operator-=(nfrac other) {
        return *this = reduced(wide(numerator_) * other.denominator_
                               - wide(other.numerator_) * denominator_,
                               wide(denominator_) * other.denominator_);
    }
    constexpr nfrac& operator*=(nfrac other) {
        return *this = reduced(wide(numerator_) * other.numerator_,
                               wide(denominator_) * other.denominator_);
    }
    constexpr nfrac& operator/=(nfrac other) {
        // Do not form other.inv(): it can overflow even when the quotient fits.
        return *this = reduced(wide(numerator_) * other.denominator_,
                               wide(denominator_) * other.numerator_);
    }
    friend constexpr nfrac operator+(nfrac a, nfrac b) { return a += b; }
    friend constexpr nfrac operator-(nfrac a, nfrac b) { return a -= b; }
    friend constexpr nfrac operator*(nfrac a, nfrac b) { return a *= b; }
    friend constexpr nfrac operator/(nfrac a, nfrac b) { return a /= b; }
    friend constexpr bool operator==(nfrac, nfrac) = default;
    friend constexpr auto operator<=>(nfrac a, nfrac b) {
        return wide(a.numerator_) * b.denominator_ <=> wide(b.numerator_) * a.denominator_;
    }
    friend ostream& operator<<(ostream& out, nfrac value) {
        return out << static_cast<long long>(value.numerator_) << '/'
                   << static_cast<long long>(value.denominator_);
    }

private:
    using wide = __int128_t;
    I numerator_ = 0, denominator_ = 1;

    static constexpr nfrac reduced(wide numerator, wide denominator) {
        if (!denominator) throw domain_error("nfrac: zero denominator or division by zero");
        if (denominator < 0) numerator = -numerator, denominator = -denominator;
        wide a = numerator < 0 ? -numerator : numerator, b = denominator;
        while (b) {
            wide remainder = a % b;
            a = b;
            b = remainder;
        }
        numerator /= a;
        denominator /= a;
        if (numerator < numeric_limits<I>::min() || numerator > numeric_limits<I>::max()
            || denominator > numeric_limits<I>::max())
            throw overflow_error("nfrac: reduced numerator or denominator out of range");
        nfrac result;
        result.numerator_ = static_cast<I>(numerator);
        result.denominator_ = static_cast<I>(denominator);
        return result;
    }
};
