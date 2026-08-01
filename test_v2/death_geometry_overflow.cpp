#include "common.hpp"

int main() {
    npoint<long long> point{LLONG_MIN, LLONG_MIN};
    return ndot(point, point) != 0;
}
