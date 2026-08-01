#include "common.hpp"

struct nnoncopy_movement {
    int calls = 0;
    nnoncopy_movement() = default;
    nnoncopy_movement(const nnoncopy_movement&) = delete;
    void operator()(int) { ++calls; }
};

int main() {
    nvector<int> values{1, 2, 1, 3, 2, 4, 1};
    nvector<ninterval_query> queries{{0, 3, 0}, {1, 6, 1}, {2, 7, 2}, {4, 4, 3}};
    nvector<int> answer(queries.len());
    array<int, 5> frequency{};
    int distinct = 0;
    auto add = [&](int index) {
        if (frequency[values[index]]++ == 0)
            ++distinct;
    };
    auto remove = [&](int index) {
        if (--frequency[values[index]] == 0)
            --distinct;
    };
    nrun_mo(queries, values.len(), add, remove, [&](int id) { answer[id] = distinct; });
    ntest((answer == nvector<int>{2, 4, 4, 0}));

    nnoncopy_movement movement;
    int answered = 0;
    nrun_mo(queries, values.len(), movement, movement, [&](int) { ++answered; });
    ntest(movement.calls > 0 && answered == queries.len());
}
