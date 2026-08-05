#include "common.hpp"

int main() {
    auto huge = nview(INT_MAX, [](int) { return 0; });
    return nwindows(huge, 0).len();
}
