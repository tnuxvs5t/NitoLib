#include "../src-v3/io.hpp"

using clock_type = chrono::steady_clock;

template <class F>
long long timed(F&& work) {
    auto start = clock_type::now();
    invoke(forward<F>(work));
    return chrono::duration_cast<chrono::milliseconds>(clock_type::now() - start).count();
}

int main() {
    constexpr int n = 500000;
    vector<long long> values(n);
    uint64_t state = 1;
    string input;
    input.reserve(8'000'000);
    char buffer[32];
    for (long long& value : values) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        value = static_cast<long long>(state % 2'000'000'001ULL) - 1'000'000'000LL;
        auto [end, error] = to_chars(buffer, buffer + sizeof(buffer), value);
        if (error != errc{}) return 2;
        input.append(buffer, end);
        input.push_back(' ');
    }

    uint64_t standard_checksum = 0, nitori_checksum = 0;
    auto standard_read_ms = timed([&] {
        istringstream in(input);
        long long value;
        for (int i = 0; i < n; ++i) in >> value, standard_checksum += uint64_t(value);
        if (!in) abort();
    });
    auto nitori_read_ms = timed([&] {
        istringstream in(input);
        long long value;
        for (int i = 0; i < n; ++i) {
            if (!nread(in, value)) abort();
            nitori_checksum += uint64_t(value);
        }
    });

    string standard_output, nitori_output;
    auto standard_write_ms = timed([&] {
        ostringstream out;
        for (long long value : values) out << value << ' ';
        standard_output = move(out).str();
    });
    auto nitori_write_ms = timed([&] {
        ostringstream out;
        for (long long value : values) nwrite(out, value), out.put(' ');
        nitori_output = move(out).str();
    });

    if (standard_checksum != nitori_checksum || standard_output != nitori_output) return 3;
    cout << "items=" << n << " bytes=" << input.size()
         << " std_read_ms=" << standard_read_ms << " nitori_read_ms=" << nitori_read_ms
         << " std_write_ms=" << standard_write_ms << " nitori_write_ms=" << nitori_write_ms
         << " checksum=" << nitori_checksum << '\n';
}
