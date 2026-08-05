#include "common.hpp"

int main() {
    auto invalid = nrange(LLONG_MIN, LLONG_MAX);
    return invalid.len();
}
