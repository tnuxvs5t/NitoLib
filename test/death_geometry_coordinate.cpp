#include "common.hpp"

int main() {
    __uint128_t huge = __uint128_t{1} << 127;
    npoint<__uint128_t> point{huge, 0};
    return ndot(point, point) != 0;
}
