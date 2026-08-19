#pragma once
#include "func.hpp"
#include "io.hpp"

namespace nitori_debug_detail { struct printer; }

/*
Public customization facade.  A user-defined type may provide an unqualified
`ndebug_repr(ndebug_writer&, const T&)` in its own namespace; lookup is intentionally
ADL-based, so debug.hpp does not maintain a type registry.  `value` re-enters the normal
recursive renderer, `object` supplies stable field punctuation, and `raw` is reserved for
literal punctuation or already-formatted labels.
*/
struct ndebug_writer {
private:
    nitori_debug_detail::printer* state;
    explicit ndebug_writer(nitori_debug_detail::printer* source) : state(source) {}
    friend struct nitori_debug_detail::printer;

public:
    template <class T>
    void value(const T& value);

    void raw(string_view text);

    template <class F>
    void object(string_view type, F&& fields);
};

namespace nitori_debug_detail {

/*
repr recursively renders one value without copying its containing descriptor.  Views
are evaluated once per position, from left to right.  Functions evaluate each key once
and invoke their evaluator once with that same key; either operation may have caller-
visible effects when the descriptor is stateful.  Borrowed owners must remain alive.
*/
struct printer {
    ostream& out;

    void escaped(unsigned char character, char quote) {
        switch (character) {
        case '\0': out << "\\0"; return;
        case '\\': out << "\\\\"; return;
        case '\a': out << "\\a"; return;
        case '\b': out << "\\b"; return;
        case '\f': out << "\\f"; return;
        case '\n': out << "\\n"; return;
        case '\r': out << "\\r"; return;
        case '\t': out << "\\t"; return;
        case '\v': out << "\\v"; return;
        default: break;
        }
        if (character == static_cast<unsigned char>(quote)) {
            out.put('\\').put(char(character));
        } else if (character >= 0x20 && character != 0x7f) {
            out.put(char(character));
        } else {
            static constexpr char digit[] = "0123456789abcdef";
            out << "\\x" << digit[character >> 4] << digit[character & 15];
        }
    }

    void quoted(string_view text, char quote) {
        out.put(quote);
        for (unsigned char character : text) escaped(character, quote);
        out.put(quote);
    }

    void raw(string_view text) {
        out.write(text.data(), streamsize(text.size()));
    }

    void repr(bool value) { out << (value ? "true" : "false"); }
    void repr(char value) { quoted(string_view(&value, 1), '\''); }
    void repr(nullptr_t) { out << "nullptr"; }
    void repr(const string& value) { quoted(value, '"'); }
    void repr(string_view value) { quoted(value, '"'); }

    void repr(const char* value) {
        if (value) quoted(value, '"');
        else repr(nullptr);
    }

    void repr(char* value) { repr(static_cast<const char*>(value)); }

    template <size_t N>
    void repr(const char (&value)[N]) {
        size_t length = N;
        if (length && value[length - 1] == '\0') --length;
        quoted(string_view(value, length), '"');
    }

    template <class T, class A>
    void repr(const vector<T, A>& values) {
        out.put('[');
        for (size_t i = 0; i < values.size(); ++i) {
            if (i) out << ", ";
            repr(values[i]);
        }
        out.put(']');
    }

    template <class A, class B>
    void repr(const pair<A, B>& value) {
        out.put('(');
        repr(value.first);
        out << ", ";
        repr(value.second);
        out.put(')');
    }

    template <class... T>
    void repr(const tuple<T...>& value) {
        out.put('(');
        size_t position = 0;
        apply([&](auto&&... item) {
            ((position++ ? void(out << ", ") : void(), repr(item)), ...);
        }, value);
        if constexpr (sizeof...(T) == 1) out.put(',');
        out.put(')');
    }

    template <class A>
    void repr(const nview<A>& view) {
        out << "nview[";
        for (nidx_t i = 0; i < view.len(); ++i) {
            if (i) out << ", ";
            decltype(auto) value = view[i];
            repr(value);
        }
        out.put(']');
    }

    template <class D, class F>
    void repr(const nfunc<D, F>& function) {
        out << "nfunc[";
        for (nidx_t i = 0; i < function.len(); ++i) {
            if (i) out << ", ";
            out.put('(');
            decltype(auto) key = function.key(i);
            repr(key);
            out << ", ";
            decltype(auto) value = invoke(function.eval, forward<decltype(key)>(key));
            repr(value);
            out.put(')');
        }
        out.put(']');
    }

    template <class T>
    void repr(const T& value) {
        using U = remove_cv_t<T>;
        if constexpr (requires(ndebug_writer& writer, const T& item) {
                          ndebug_repr(writer, item);
                      }) {
            ndebug_writer writer{this};
            ndebug_repr(writer, value);
        } else if constexpr (numeric_limits<U>::is_integer) {
            nwrite(out, value);
        } else if constexpr (is_enum_v<U>) {
            if constexpr (requires { out << value; }) out << value;
            else repr(to_underlying(value));
        } else if constexpr (requires { out << value; }) {
            out << value;
        } else {
            static_assert(sizeof(T) == 0,
                          "ndebug cannot render this leaf; define ostream operator<<");
        }
    }

    void top(const string& value) { raw(value); }
    void top(string_view value) { raw(value); }

    void top(const char* value) {
        if (value) out << value;
        else repr(nullptr);
    }

    void top(char* value) { top(static_cast<const char*>(value)); }

    template <size_t N>
    void top(const char (&value)[N]) {
        size_t length = N;
        if (length && value[length - 1] == '\0') --length;
        raw(string_view(value, length));
    }

    template <class T>
    void top(const T& value) { repr(value); }
};

} // namespace nitori_debug_detail

inline void ndebug_writer::raw(string_view text) {
    state->raw(text);
}

template <class T>
inline void ndebug_writer::value(const T& value) {
    state->repr(value);
}

template <class F>
inline void ndebug_writer::object(string_view type, F&& fields) {
    auto& printer = *state;
    printer.raw(type);
    printer.raw("{");
    bool first = true;
    auto field = [&](string_view name, const auto& value) {
        if (!first) printer.raw(", ");
        first = false;
        printer.raw(name);
        printer.raw("=");
        printer.repr(value);
    };
    invoke(forward<F>(fields), field);
    printer.raw("}");
}

/* Python-print-like top-level strings, one space between values, then newline+flush. */
template <class... T>
ostream& ndebug(ostream& out, const T&... value) {
    if constexpr (sizeof...(T)) {
        nitori_debug_detail::printer printer{out};
        bool first = true;
        auto emit = [&](const auto& item) {
            if (!first) out.put(' ');
            first = false;
            printer.top(item);
        };
        (emit(value), ...);
    }
    out.put('\n');
    out.flush();
    return out;
}

inline ostream& ndebug() { return ndebug(cerr); }

template <class T, class... U>
requires (!requires(T& value) { static_cast<ostream&>(value); })
ostream& ndebug(const T& first, const U&... rest) {
    return ndebug(cerr, first, rest...);
}
