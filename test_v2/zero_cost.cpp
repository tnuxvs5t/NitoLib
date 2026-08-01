#include "common.hpp"

static size_t allocations = 0;

void* operator new(size_t size) {
    ++allocations;
    if (void* memory = malloc(size))
        return memory;
    throw bad_alloc();
}

void operator delete(void* memory) noexcept { free(memory); }
void operator delete(void* memory, size_t) noexcept { free(memory); }

int main() {
    int raw[]{7, 6, 5, 4, 3, 2, 1, 0};
    int* pointer = raw;
    size_t before = allocations;

    nspan<int> contiguous(raw);
    auto indirect = nview(4, [pointer](int i) -> int& { return pointer[2 * i]; });
    auto strided = nstride(contiguous, 1, 4, 2);
    auto borrowed = nall(contiguous);

    static_assert(sizeof(contiguous) <= 2 * sizeof(void*));
    static_assert(sizeof(indirect) <= 2 * sizeof(void*));
    static_assert(sizeof(borrowed) <= 2 * sizeof(void*));

    nsort(indirect);
    nsort(strided);
    nreverse_inplace(borrowed);

    ntest(allocations == before);
    ntest(raw[0] == 6 && raw[7] == 1);
}
