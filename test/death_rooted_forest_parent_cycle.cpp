#include "common.hpp"

int main() {
    nrooted_forest<int> invalid(nvector<int>{1, 0});
    return invalid.len();
}
