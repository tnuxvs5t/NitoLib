#include "common.hpp"

int main() {
    auto invalid = nfunc_bind(nrange(3), nrange(2));
    return invalid.len();
}
