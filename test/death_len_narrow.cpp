#include "common.hpp"

struct nhuge_indexed {
    __uint128_t len() const { return (__uint128_t{1} << 64) + 1; }
    int operator[](int) const { return 0; }
};

int main() { return nlen(nhuge_indexed{}); }
