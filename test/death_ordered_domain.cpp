#include "common.hpp"

int main() {
    nset_fhq<int> left{1}, right{2};
    left.merge_from(move(right));
}
