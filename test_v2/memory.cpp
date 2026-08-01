#include "common.hpp"

struct record {
    int parent, value;
    record(int parent, int value) : parent(parent), value(value) {}
};

int main() {
    nscratch<int> scratch;
    auto first = scratch.filled(8, 7);
    ntest(nfold(first) == 56);
    auto second = scratch.space(4);
    ntest(second[0] == 7 && scratch.cap() >= 8);
    second[2] = 19;

    narena<record> arena;
    arena.reserve(16);
    int root = arena.make(npos, 5);
    int checkpoint = arena.mark();
    int child = arena.make(root, 9);
    ntest(arena[child].parent == root && arena[child].value == 9);
    arena.rollback(checkpoint);
    ntest(arena.len() == 1 && arena.get(child) == nullptr);
    int reused = arena.make(root, 12);
    ntest(reused == child && arena[reused].value == 12);
}
