#include "../src-v3/reftree.hpp"

using U = uint64_t;

struct count_ops {
    U count(const auto& q, nidx_t p, nidx_t h) const {
        return q[p].value << (h - q[p].height);
    }
    U join(const auto& q, nidx_t h, nidx_t l, nidx_t r) const {
        return count(q, l, h - 1) + count(q, r, h - 1);
    }
};

int main() {
    constexpr nidx_t H = 60, updates = 20000, queries = 50000;
    constexpr U end = U(1) << H, slots = U(1) << 15;
    nreftree<U, count_ops> q;
    q.reserve(updates * H + 3);
    nidx_t zero = q.leaf(0), one = q.leaf(1), odd = q.join(1, zero, one);
    if (q.nodes() != 3 || q.ops.count(q, odd, H) != end / 2) abort();
    vector<nidx_t> roots{odd}, inserted_at(slots, updates);
    roots.reserve(updates + 1);
    auto start = chrono::steady_clock::now();
    for (nidx_t i = 0; i < updates; ++i) {
        U slot = U(i) * 11939 % slots;
        inserted_at[slot] = i;
        roots.push_back(q.set(roots.back(), H, 2 * slot, one));
        if (q.ops.count(q, roots.back(), H) != end / 2 + U(i + 1)) abort();
    }
    auto updated = chrono::steady_clock::now();
    nidx_t nodes = q.nodes();
    auto count = [&](nidx_t p, nidx_t h) { return q.ops.count(q, p, h); };
    auto has = [&](nidx_t p, nidx_t h) { return count(p, h) != 0; };
    U state = 0x72656674726565ULL, checksum = 0;
    for (nidx_t i = 0; i < queries; ++i) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        nidx_t version = nidx_t(state % U(updates + 1));
        U x = (state >> 32) % (2 * slots);
        U expected = (x & 1) || inserted_at[x / 2] < version ? x : x + 1;
        U actual = q.find_first(roots[version], H, x, has);
        if (actual != expected) abort();
        if (q.kth(roots[version], H, end / 2 + U(version) - 1, count) != end - 1) abort();
        checksum = checksum * 131 + actual;
    }
    auto queried = chrono::steady_clock::now();
    if (q.nodes() != nodes || q.leaf_at(odd, 0) != zero) abort();
    if (q.paste(odd, H, U(1) << 59, 30, odd) != odd || q.nodes() != nodes) abort();
    cout << "reftree H=" << H << " updates=" << updates << " queries=" << queries
         << " index_bytes=" << sizeof(nidx_t) << " node_bytes=" << sizeof(decltype(q)::node)
         << " nodes=" << nodes << " node_storage_bytes=" << size_t(nodes) * sizeof(decltype(q)::node)
         << " update_ms=" << chrono::duration_cast<chrono::milliseconds>(updated - start).count()
         << " query_ms=" << chrono::duration_cast<chrono::milliseconds>(queried - updated).count()
         << " checksum=" << checksum << '\n';
}
