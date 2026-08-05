#include "common.hpp"

struct ndistance_state {
    long long vertices, distance_sum;
};

int main() {
    ngraph_list<int> tree(6);
    tree.add2(0, 1);
    tree.add2(1, 2);
    tree.add2(1, 3);
    tree.add2(3, 4);
    tree.add2(3, 5);

    auto answer = nreroot(
        tree, ndistance_state{0, 0},
        [](ndistance_state a, ndistance_state b) {
            return ndistance_state{a.vertices + b.vertices, a.distance_sum + b.distance_sum};
        },
        [](ndistance_state aggregate, int) {
            ++aggregate.vertices;
            return aggregate;
        },
        [](ndistance_state state, int, int) {
            state.distance_sum += state.vertices;
            return state;
        });

    nvector<long long> expected{11, 7, 11, 7, 11, 11};
    for (int vertex = 0; vertex < tree.vertices(); ++vertex) {
        ntest(answer[vertex].vertices == 6);
        ntest(answer[vertex].distance_sum == expected[vertex]);
    }
}
