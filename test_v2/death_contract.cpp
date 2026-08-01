#include "common.hpp"

int main() {
    nspan<int> invalid(nullptr, 1);
    return invalid.len();
}
