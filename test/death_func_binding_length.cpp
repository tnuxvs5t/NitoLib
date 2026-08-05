#include "common.hpp"

int main() {
    auto invalid = nfunc(nrange(3), nrange(2));
    return invalid.len();
}
