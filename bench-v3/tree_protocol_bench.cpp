#include "../src-v3/segment.hpp"
#include "../src-v3/fhq.hpp"

template <class F>
long long timed(F&& work) {
    auto start = chrono::steady_clock::now();
    work();
    return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count();
}

int main() {
    constexpr nidx_t n = 200000, operations = 200000;
    vector<long long> initial(n), oracle(n);
    iota(initial.begin(), initial.end(), 0LL);
    oracle = initial;
    struct operation { nidx_t left, right; long long delta; };
    vector<operation> workload;
    vector<long long> expected;
    uint64_t state = 223391;
    for (nidx_t i = 0; i < operations; ++i) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        nidx_t left = nidx_t(state % n), right = min(n, left + nidx_t((state >> 32) % 1025));
        long long delta = static_cast<long long>(state % 21) - 10;
        workload.push_back({left, right, delta});
        if (i & 1) expected.push_back(accumulate(oracle.begin() + left, oracle.begin() + right, 0LL));
        else for (nidx_t j = left; j < right; ++j) oracle[j] += delta;
    }
    unique_ptr<nlazy_addsum<long long>> seg;
    auto build_ms = timed([&] { seg = make_unique<nlazy_addsum<long long>>(nall(initial)); });
    uint64_t checksum = 0;
    auto lazy_ms = timed([&] {
        nidx_t query = 0;
        for (nidx_t i = 0; i < operations; ++i) {
            auto [left, right, delta] = workload[i];
            if (i & 1) {
                auto actual = seg->fold(left, right);
                if (actual != expected[query++]) abort();
                checksum += uint64_t(actual);
            } else seg->apply(left, right, delta);
        }
    });
    if (seg->fold() != accumulate(oracle.begin(), oracle.end(), 0LL)) abort();
    size_t storage = seg->tree.size() * sizeof(long long) +
                     seg->ops.lazy.size() * sizeof(long long) + seg->ops.pending.size();

    nfhq<long long> q;
    q.reserve(n);
    nidx_t root = q.build(nall(initial));
    uint64_t expected_width = 0, width_sum = 0;
    for (const auto& op : workload) expected_width += op.right - op.left;
    auto fhq_ms = timed([&] {
        for (const auto& op : workload)
            root = q.edit(root, op.left, op.right, [&](auto& tree, nidx_t middle) {
                width_sum += tree.size(middle);
                return middle;
            });
    });
    if (width_sum != expected_width || q.size(root) != n) abort();
    for (nidx_t i = 0; i < n; i += 97) {
        if (q[q.kth(root, i)].value != initial[i]) abort();
        checksum += uint64_t(initial[i]);
    }
    cout << "n=" << n << " operations=" << operations << " index_bytes=" << sizeof(nidx_t)
         << " lazy_build_ms=" << build_ms << " lazy_work_ms=" << lazy_ms
         << " lazy_storage_bytes=" << storage
         << " old_layout_bytes=" << size_t(2) * seg->base * (2 * sizeof(long long) + 1)
         << " fhq_edit_ms=" << fhq_ms << " checksum=" << checksum + width_sum << '\n';
}
