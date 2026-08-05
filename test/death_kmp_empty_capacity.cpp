#include "common.hpp"

int main() {
    auto huge = nview(INT_MAX, [](int) { return 0; });
    nvector<int> empty;
    return nkmp_find(huge, empty).len();
}
