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

    nview<int> contiguous(raw);
    auto indirect = nview(4, [pointer](int i) -> int& { return pointer[2 * i]; });
    auto strided = nstride(contiguous, 1, contiguous.len(), 2);
    auto borrowed = nall(contiguous);

    int source[]{10, 20, 30, 40};
    int destination[4]{};
    auto source_view = nview(source);
    auto destination_view = nview(destination);

    static_assert(sizeof(contiguous) <= 2 * sizeof(void*));
    static_assert(sizeof(indirect) <= 2 * sizeof(void*));
    static_assert(sizeof(borrowed) <= 2 * sizeof(void*));

    nsort(indirect);
    nsort(strided);
    nreverse_inplace(borrowed);
    nassign(destination_view, source_view);
    nfill(nsub(destination_view, 1, 3), 5);
    nswap_ranges(nsub(destination_view, 0, 2), nsub(source_view, 2, 4));
    ntest(ncount(destination_view, 5) == 1);
    ntest(nargmax(destination_view) != npos);

    ntest(allocations == before);
    ntest(raw[0] == 6 && raw[7] == 1);
}
