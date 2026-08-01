#include "common.hpp"

int main() {
    narray<int, 1> values({1}, 7);
    return values(1LL << 32);
}
