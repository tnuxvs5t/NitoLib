#include "common.hpp"

int main() {
    return nfirst_true(4, 3, [](int) { return true; });
}
