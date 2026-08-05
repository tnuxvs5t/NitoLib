#include "common.hpp"

template <class T>
concept ninput_readable = requires(ninput& input, T& value) { input.read(value); };

int main() {
    static_assert(!ninput_readable<bool>);

    FILE* input_file = tmpfile();
    ntest(input_file != nullptr);
    string input = "-9223372036854775808 18446744073709551615 -170141183460469231731687303715884105728 hello Z";
    ntest(fwrite(input.data(), 1, input.size(), input_file) == input.size());
    rewind(input_file);
    ninput input_stream(input_file);
    long long minimum;
    unsigned long long maximum;
    __int128_t wide_minimum;
    string word;
    char letter;
    ntest(input_stream.read(minimum) && input_stream.read(maximum) && input_stream.read(wide_minimum));
    ntest(input_stream.read(word) && input_stream.read(letter));
    ntest(minimum == LLONG_MIN && maximum == ULLONG_MAX);
    ntest(wide_minimum == numeric_limits<__int128_t>::lowest());
    ntest(word == "hello" && letter == 'Z');
    ntest(!input_stream.read(minimum));
    fclose(input_file);

    FILE* output_file = tmpfile();
    ntest(output_file != nullptr);
    {
        noutput output(output_file);
        output << LLONG_MIN << ' ' << ULLONG_MAX << ' ' << numeric_limits<__int128_t>::lowest();
        output << ' ' << string("kappa") << '\n';
        output.flush();
    }
    rewind(output_file);
    array<char, 256> buffer{};
    size_t bytes = fread(buffer.data(), 1, buffer.size() - 1, output_file);
    ntest(bytes > 0);
    ntest(string(buffer.data()) == input.substr(0, input.find(" hello")) + " kappa\n");
    fclose(output_file);
}
