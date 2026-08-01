namespace ni {
template <class T>
inline constexpr bool nio_integer =
    (!same_as<remove_cv_t<T>, bool>) &&
    (is_integral_v<T> || same_as<T, __int128_t> || same_as<T, __uint128_t>);

template <class T> struct nio_unsigned_type {
    using type = make_unsigned_t<T>;
};
template <> struct nio_unsigned_type<__int128_t> {
    using type = __uint128_t;
};
template <> struct nio_unsigned_type<__uint128_t> {
    using type = __uint128_t;
};
template <class T> using nio_unsigned_t = typename nio_unsigned_type<T>::type;
} // namespace ni

class ninput {
    static constexpr int capacity_ = 1 << 16;
    FILE* file_ = nullptr;
    array<unsigned char, capacity_> buffer_{};
    int position_ = 0, size_ = 0;

    int get() {
        if (position_ == size_) {
            size_ = int(fread(buffer_.data(), 1, buffer_.size(), file_));
            position_ = 0;
            if (!size_)
                return EOF;
        }
        return buffer_[position_++];
    }

    int token_start() {
        int character;
        do
            character = get();
        while (character != EOF && character <= ' ');
        return character;
    }

  public:
    explicit ninput(FILE* file = stdin) : file_(file) { npre(file != nullptr); }

    template <class T>
        requires ni::nio_integer<T>
    bool read(T& value) {
        int character = token_start();
        if (character == EOF)
            return false;
        bool negative = character == '-';
        if (negative || character == '+')
            character = get();
        npre('0' <= character && character <= '9');
        using U = ni::nio_unsigned_t<T>;
        __uint128_t magnitude = 0;
        do {
            unsigned digit = unsigned(character - '0');
            constexpr __uint128_t wide_limit = ~__uint128_t{};
            npre(magnitude <= (wide_limit - digit) / 10);
            magnitude = magnitude * 10 + digit;
            character = get();
        } while ('0' <= character && character <= '9');
        npre(character == EOF || character <= ' ');

        if constexpr (is_signed_v<T> || same_as<T, __int128_t>) {
            __uint128_t positive_limit = __uint128_t(numeric_limits<T>::max());
            __uint128_t negative_limit = positive_limit + 1;
            npre(magnitude <= (negative ? negative_limit : positive_limit));
            if (negative) {
                if (magnitude == negative_limit)
                    value = numeric_limits<T>::lowest();
                else
                    value = -T(magnitude);
            } else {
                value = T(magnitude);
            }
        } else {
            npre(!negative && magnitude <= __uint128_t(numeric_limits<U>::max()));
            value = T(U(magnitude));
        }
        return true;
    }

    bool read(string& value) {
        int character = token_start();
        if (character == EOF)
            return false;
        value.clear();
        do {
            value.push_back(char(character));
            character = get();
        } while (character != EOF && character > ' ');
        return true;
    }

    bool read(char& value) {
        int character = token_start();
        if (character == EOF)
            return false;
        value = char(character);
        return true;
    }

    template <class T> ninput& operator>>(T& value) {
        npre(read(value));
        return *this;
    }
};

class noutput {
    static constexpr int capacity_ = 1 << 16;
    FILE* file_ = nullptr;
    array<char, capacity_> buffer_{};
    int size_ = 0;

    void put(char character) {
        if (size_ == capacity_)
            flush();
        buffer_[size_++] = character;
    }

  public:
    explicit noutput(FILE* file = stdout) : file_(file) { npre(file != nullptr); }
    noutput(const noutput&) = delete;
    noutput& operator=(const noutput&) = delete;
    ~noutput() { flush(); }

    void flush() {
        if (size_) {
            size_t written = fwrite(buffer_.data(), 1, size_t(size_), file_);
            npre(written == size_t(size_));
            size_ = 0;
        }
    }

    void write(char value) { put(value); }
    void write(string_view value) {
        for (char character : value)
            put(character);
    }
    void write(const string& value) { write(string_view(value)); }
    void write(const char* value) { write(string_view(value)); }

    template <class T>
        requires ni::nio_integer<T>
    void write(T value) {
        using U = ni::nio_unsigned_t<T>;
        U magnitude;
        if constexpr (is_signed_v<T> || same_as<T, __int128_t>) {
            if (value < 0) {
                put('-');
                U encoded = U(value);
                magnitude = U{} - encoded;
            } else {
                magnitude = U(value);
            }
        } else {
            magnitude = U(value);
        }
        char digits[64];
        int count = 0;
        do {
            digits[count++] = char('0' + magnitude % 10);
            magnitude /= 10;
        } while (magnitude);
        while (count)
            put(digits[--count]);
    }

    template <class T> noutput& operator<<(const T& value) {
        write(value);
        return *this;
    }
};

inline ninput nin;
inline noutput nout;

template <class... T> bool nread(T&... values) { return (nin.read(values) && ...); }

template <class... T> void nprint(const T&... values) {
    bool first = true;
    auto write = [&](const auto& value) {
        if (!first)
            nout.write(' ');
        first = false;
        nout.write(value);
    };
    (write(values), ...);
}

template <class... T> void nprintln(const T&... values) {
    nprint(values...);
    nout.write('\n');
}
