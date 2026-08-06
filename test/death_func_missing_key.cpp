#include "common.hpp"

int main() {
    auto function = nfunc_bind(nvector<int>{10, 20}, nvector<int>{1, 2});
    return function(30);
}
