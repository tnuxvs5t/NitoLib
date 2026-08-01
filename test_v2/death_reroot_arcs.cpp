#include "common.hpp"

int main() {
    ngraph_list<int> invalid(2);
    invalid.add(0, 1);
    invalid.add(0, 1);
    auto answer = nreroot(
        invalid, 0, [](int a, int b) { return a + b; },
        [](int aggregate, int) { return aggregate + 1; },
        [](int state, int, int) { return state; });
    return answer.len();
}
