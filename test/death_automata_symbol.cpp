#include "common.hpp"

int main() {
    ntrie<2> trie;
    return trie.add(nvector<unsigned long long>{1ULL << 32});
}
