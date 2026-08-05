#include "common.hpp"

int main() {
    narray<int, 2> invalid({0, -1});
    return invalid.len();
}
