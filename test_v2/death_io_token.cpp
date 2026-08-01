#include "common.hpp"

int main() {
    FILE* file = tmpfile();
    ntest(file != nullptr);
    constexpr string_view token = "12x";
    ntest(fwrite(token.data(), 1, token.size(), file) == token.size());
    rewind(file);
    ninput input(file);
    int value = 0;
    bool read = input.read(value);
    fclose(file);
    return read;
}
