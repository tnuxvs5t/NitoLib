#include "common.hpp"

int main() {
    nview<int> invalid(nullptr, 1);
    return invalid.len();
}
