#include "common.hpp"

int main() {
    ndynamic_seg<int> left(0, 8), right(0, 8);
    left.set(1, 1);
    right.set(2, 2);
    left.merge_from(move(right));
}
