#include "common.hpp"

int main() {
    nseq_fhq<int> left{1}, right{2};
    left.merge_from(move(right));
}
