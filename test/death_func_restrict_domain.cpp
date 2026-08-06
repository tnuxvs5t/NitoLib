#include "common.hpp"

int main() {
    auto square = nfunc_value(nrange(4), [](int value) { return value * value; });
    auto restricted = nrestrict(square, nvector<int>{1, 3});
    return restricted(2);
}
