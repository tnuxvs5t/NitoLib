#include "common.hpp"

int main() {
    auto function = nfunc(nvector<int>{10, 20}, nvector<int>{1, 2});
    return function(30);
}
