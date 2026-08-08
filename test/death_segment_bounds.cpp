#include "common.hpp"

int main() {
    using tree_type = ndynamic_seg<int>;
    auto domain = tree_type(0, 8).domain();
    tree_type left(domain, 0, 8), right(domain, 0, 4);
    left.set(1, 1);
    right.set(2, 2);
    left.merge_from(move(right));
}
