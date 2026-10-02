#include "../include/sequence.hpp"
#include "../include/hld.hpp"
#include "../include/graph.hpp"
#include "../../src-v3/discrete.hpp"
#include "../../src-v3/graph.hpp"
#include <cstdio>
#include <cstdlib>

static size_t allocated = 0, allocations = 0;
[[gnu::noinline]] void* operator new(size_t bytes) {
    if (void* p = std::malloc(bytes ? bytes : 1)) { allocated += bytes; ++allocations; return p; }
    throw std::bad_alloc();
}
[[gnu::noinline]] void operator delete(void* p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete(void* p, size_t) noexcept { std::free(p); }

int main() {
    constexpr int n = 50000;
    std::vector<int> values(n);
    std::iota(values.begin(), values.end(), 0);
    auto old_plan = norder(nall(values));
    allocations = allocated = 0;
    auto old_slice = nslice(old_plan, 0, 10);
    size_t old_bytes = allocated, old_allocations = allocations;
    if (old_slice[3] != 3) std::abort();
    allocations = allocated = 0;
    auto old_borrowed = nslice(nall(old_plan), 0, 10);
    size_t old_borrowed_bytes = allocated;
    if (old_borrowed[3] != 3 || old_borrowed_bytes) std::abort();
    auto order = argsort(values);
    allocations = allocated = 0;
    auto selected = gather(std::span(values), std::span<const int>(order).subspan(0, 10));
    size_t new_bytes = allocated, new_allocations = allocations;
    if (selected[3] != 3 || new_bytes) std::abort();
    allocations = allocated = 0;
    auto anchors = nanchors(nmap(nrange(n), std::identity{}), nrange(n));
    size_t anchor_bytes = allocated, anchor_allocations = allocations;
    if (anchors(123) != 123) std::abort();

    constexpr int vertices = 10000;
    std::vector<std::vector<int>> adj(vertices);
    for (int v = 1; v < vertices; ++v) {
        adj[(v - 1) / 2].push_back(v);
        adj[v].push_back((v - 1) / 2);
    }
    auto next = [&](int v) -> const auto& { return adj[v]; };
    auto old_graph = ngraph{nrange(vertices), next};
    auto old_distance = nbfs(old_graph, 0);
    auto new_result = bfs(vertices, 0, next);
    if (old_distance != new_result.distance) std::abort();
    long long checksum = std::accumulate(new_result.distance.begin(), new_result.distance.end(), 0LL);
    hld tree(vertices, 0, next);
    allocations = allocated = 0;
    long long pieces = 0;
    for (int v = 0; v < vertices; ++v)
        tree.path(v, vertices - 1, [&](path_piece p) { pieces += p.right - p.left; });
    if (allocations) std::abort();
    std::printf("{\"n\":%d,\"v3_slice_bytes\":%zu,\"v3_slice_allocations\":%zu,"
                "\"v3_borrowed_slice_bytes\":%zu,\"lab_slice_bytes\":%zu,\"lab_slice_allocations\":%zu,"
                "\"v3_mapped_anchor_bytes\":%zu,\"v3_mapped_anchor_allocations\":%zu,"
                "\"hld_path_allocations\":%zu,\"bfs_checksum\":%lld,\"path_vertex_checksum\":%lld}\n",
                n, old_bytes, old_allocations, old_borrowed_bytes, new_bytes, new_allocations,
                anchor_bytes, anchor_allocations, allocations, checksum, pieces);
}
