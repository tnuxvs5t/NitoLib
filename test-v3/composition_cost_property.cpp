#include "../src-v3/discrete.hpp"
#include "../src-v3/ds.hpp"
#include "../src-v3/tree.hpp"

#define CHECK(x) do { if (!(x)) abort(); } while (false)

static size_t allocations = 0;

[[gnu::noinline]] void* operator new(size_t bytes) {
    if (void* pointer = malloc(bytes ? bytes : 1)) {
        ++allocations;
        return pointer;
    }
    throw bad_alloc();
}
[[gnu::noinline]] void operator delete(void* pointer) noexcept { free(pointer); }
[[gnu::noinline]] void operator delete(void* pointer, size_t) noexcept { free(pointer); }

struct first_nonzero {
    nidx_t operator()(nidx_t a, nidx_t b) const { return a ? a : b; }
};

int main() {
    mt19937 rng(91827);
    for (nidx_t round = 0; round < 300; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 200);
        vector<nidx_t> values(n);
        for (auto& value : values) value = nidx_t(rng() % 100);
        auto oracle = values;
        sort(oracle.begin(), oracle.end());
        auto blocks = nblocks(norder(nall(values)), 1);
        size_t before = allocations;
        for (nidx_t i = 0; i < n; ++i) CHECK(blocks[i][0] == oracle[i]);
        CHECK(allocations == before);

        auto selected = npositions(nall(values), [](nidx_t value) { return value % 3 == 0; });
        nidx_t at = 0;
        for (nidx_t i = 0; i < n; ++i)
            if (values[i] % 3 == 0) CHECK(selected[at++] == i);
        CHECK(at == nlen(selected));

        auto ordered = norder(nall(values));
        before = allocations;
        CHECK(nlower(ordered, oracle.back()) ==
              nidx_t(lower_bound(oracle.begin(), oracle.end(), oracle.back()) - oracle.begin()));
        CHECK(nupper(ordered, oracle.back()) == n);
        CHECK(ncontains(ordered, oracle.front()));
        CHECK(naccumulate(ordered, 0LL) == accumulate(oracle.begin(), oracle.end(), 0LL));
        CHECK(allocations == before);

        nsparse_table table(nall(values), first_nonzero{});
        for (nidx_t left = 0; left < n; ++left) {
            nidx_t expected = 0;
            for (nidx_t right = left + 1; right <= n; ++right) {
                if (!expected) expected = values[right - 1];
                CHECK(table.fold(left, right) == expected);
            }
        }
    }

    auto detached = [] {
        vector<nidx_t> values{4, 1, 3, 2};
        auto source = ntabulate(4, [values = move(values)](nidx_t i) { return values[i]; });
        auto blocks = nblocks(norder(move(source)), 2);
        return blocks[1];
    }();
    CHECK(detached[0] == 3 && detached[1] == 4);

    auto function = nfunc{nrange(10, 16), [base = make_unique<nidx_t>(100)](nidx_t key) {
        return *base + key;
    }};
    CHECK(naccumulate(function, 0) == 675);
    CHECK(ncollect(function).size() == 6);
    auto child = nblocks(move(function), 2)[1];
    CHECK(child.key(0) == 12 && child[1] == 113 && child(12) == 112);
    auto keys = nkeys(nblocks(nfunc{nrange(3), identity{}}, 2)[0]);
    CHECK(keys[1] == 1 && keys.inverse(1) == 1);

    auto movable_view = ntabulate(4, [state = make_unique<nidx_t>(0)](nidx_t) mutable {
        return ++*state;
    });
    auto windows = nwindows(move(movable_view), 2);
    CHECK(windows[0][0] == 1 && windows[1][0] == 2);

    vector<nidx_t> domain{30, 10, 20};
    auto inverse = ninvert(nall(domain));
    auto borrowed = nall(inverse);
    size_t before = allocations;
    auto anchored = nanchors(borrowed, nrange(3));
    CHECK(anchored(10) == 1 && allocations == before);

    vector<vector<nidx_t>> adjacency{{1, 2}, {3}, {}, {}};
    auto graph = ngraph{nrange(4), [&](nidx_t vertex) -> auto& { return adjacency[vertex]; }};
    auto hld = nhld(nroot(graph, nrange(1)));
    before = allocations;
    nidx_t count = 0;
    hld.visit_path(3, 2, [&](npath_piece piece) { count += piece.right - piece.left; });
    CHECK(count == 4 && allocations == before);
}
