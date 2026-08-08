#include "common.hpp"

int main() {
    auto domain = nset_fhq<int>{}.domain();
    nset_fhq<int> left(domain), right(domain);
    left.ins(2);
    right.ins(1);
    left.merge_from(move(right));
}
