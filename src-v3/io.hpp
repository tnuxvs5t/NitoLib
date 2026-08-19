#pragma once
#include "core.hpp"

namespace nitori_io_detail {

/*
Integer-like values support zero construction, digit arithmetic, comparison with zero,
and unary minus when signed input is accepted.  Reading uses the supplied istream's
rdbuf and state, so nscan(in,...) and ordinary in >> ... may be interleaved.  EOF after
a complete token sets eofbit only; missing digits or out-of-range input also sets failbit.
*/
template <class T>
bool read_integer(istream& in, T& value) {
    static_assert(numeric_limits<T>::is_integer, "Nitori integer I/O requires an integer type");
    using C = istream::traits_type;
    auto* buffer = in.rdbuf();
    auto current = buffer->sgetc();
    while (!C::eq_int_type(current, C::eof()) && isspace(static_cast<unsigned char>(C::to_char_type(current)))) {
        buffer->snextc();
        current = buffer->sgetc();
    }
    if (C::eq_int_type(current, C::eof())) {
        in.setstate(ios::eofbit | ios::failbit);
        return false;
    }

    bool negative = false;
    char character = C::to_char_type(current);
    if (character == '+' || character == '-') {
        negative = character == '-';
        buffer->snextc();
        current = buffer->sgetc();
    }

    T result{};
    bool any = false, overflow = false;
    T maximum = numeric_limits<T>::max();
    T negative_limit = T{};
    if constexpr (numeric_limits<T>::is_signed)
        negative_limit = numeric_limits<T>::lowest();
    else if (negative)
        overflow = true;

    while (!C::eq_int_type(current, C::eof())) {
        character = C::to_char_type(current);
        if (character < '0' || character > '9') break;
        any = true;
        nidx_t digit = character - '0';
        if (!overflow) {
            if (negative) {
                if (result < (negative_limit + digit) / 10) overflow = true;
                else result = result * 10 - digit;
            } else {
                if (result > (maximum - digit) / 10) overflow = true;
                else result = result * 10 + digit;
            }
        }
        buffer->snextc();
        current = buffer->sgetc();
    }
    if (C::eq_int_type(current, C::eof())) in.setstate(ios::eofbit);
    if (!any || overflow) {
        in.setstate(ios::failbit);
        return false;
    }
    value = result;
    return true;
}

template <class T>
bool write_integer(ostream& out, T value) {
    static_assert(numeric_limits<T>::is_integer, "Nitori integer I/O requires an integer type");
    using C = ostream::traits_type;
    auto* buffer = out.rdbuf();
    char digits[numeric_limits<T>::digits10 + 3];
    nidx_t length = 0;
    bool negative = value < T{};
    do {
        T remainder = value % 10;
        if (remainder < T{}) remainder = -remainder;
        digits[length++] = char('0' + nidx_t(remainder));
        value /= 10;
    } while (value != T{});
    if (negative && C::eq_int_type(buffer->sputc('-'), C::eof())) return false;
    while (length)
        if (C::eq_int_type(buffer->sputc(digits[--length]), C::eof())) return false;
    return true;
}

} // namespace nitori_io_detail

template <class T>
bool nread(istream& in, T& value) {
    typename istream::sentry guard(in, true);
    if (!guard) return false;
    return nitori_io_detail::read_integer(in, value);
}

template <class T>
bool nread(T& value) {
    return nread(cin, value);
}

template <class T, class... U>
bool nscan(istream& in, T& first, U&... rest) {
    typename istream::sentry guard(in, true);
    if (!guard) return false;
    return nitori_io_detail::read_integer(in, first) &&
           (nitori_io_detail::read_integer(in, rest) && ...);
}

template <class T, class... U>
requires (!requires(T& value) { static_cast<istream&>(value); })
bool nscan(T& first, U&... rest) {
    return nscan(cin, first, rest...);
}

template <class T>
ostream& nwrite(ostream& out, T value) {
    typename ostream::sentry guard(out);
    if (guard && !nitori_io_detail::write_integer(out, value)) out.setstate(ios::badbit);
    return out;
}

template <class T>
ostream& nwrite(T value) {
    return nwrite(cout, value);
}

template <class... T>
ostream& nprint(ostream& out, const T&... value) {
    typename ostream::sentry guard(out);
    if (!guard) return out;
    bool first = true;
    auto emit = [&](const auto& item) {
        if (!out) return;
        if (!first && ostream::traits_type::eq_int_type(out.rdbuf()->sputc(' '),
                                                       ostream::traits_type::eof())) {
            out.setstate(ios::badbit);
            return;
        }
        first = false;
        if (!nitori_io_detail::write_integer(out, item)) out.setstate(ios::badbit);
    };
    (emit(value), ...);
    return out;
}

inline ostream& nprint() { return cout; }

template <class T, class... U>
requires (!requires(T& value) { static_cast<ostream&>(value); })
ostream& nprint(const T& first, const U&... rest) {
    return nprint(cout, first, rest...);
}

template <class... T>
ostream& nprintln(ostream& out, const T&... value) {
    nprint(out, value...);
    return out.put('\n');
}

inline ostream& nprintln() { return cout.put('\n'); }

template <class T, class... U>
requires (!requires(T& value) { static_cast<ostream&>(value); })
ostream& nprintln(const T& first, const U&... rest) {
    return nprintln(cout, first, rest...);
}
