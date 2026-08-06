#include "common.hpp"

int main() {
    nvector<int> keys{1, 2, 1};
    nvector<int> values{10, 20, 30};
    auto invalid = nfunc_bind(keys, values);
    return invalid.len();
}
