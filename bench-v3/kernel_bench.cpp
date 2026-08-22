#include "../src-v3/bag.hpp"
#include "../src-v3/discrete.hpp"
#include "../src-v3/fhq.hpp"
#include "../src-v3/graph.hpp"
#include "../src-v3/graph_store.hpp"
#include "../src-v3/link_cut.hpp"
#include "../src-v3/segment.hpp"
#include "../src-v3/wavelet.hpp"

using clock_type = chrono::steady_clock;

template <class F>
long long timed(F&& work) {
    auto start = clock_type::now();
    invoke(forward<F>(work));
    return chrono::duration_cast<chrono::milliseconds>(clock_type::now() - start).count();
}

long long peak_rss_kib() {
    ifstream status("/proc/self/status");
    string key, unit;
    long long value;
    while (status >> key) {
        if (key == "VmHWM:") return status >> value >> unit, value;
        status.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    return -1;
}

int main() {
    constexpr nidx_t n = 200000;
    uint64_t checksum = 0;
    vector<nidx_t> values(n);
    iota(values.begin(), values.end(), 1);

    vector<nidx_t> shuffled(n);
    for (nidx_t i = 0; i < n; ++i) shuffled[i] = nidx_t(1LL * i * 48271 % n);
    vector<nidx_t> direct_sorted = shuffled, projected_sorted = shuffled;
    auto direct_sort = timed([&] { ranges::sort(direct_sorted); });
    auto projected_sort = timed([&] { nsort(nall(projected_sorted)); });
    if (direct_sorted != projected_sorted) return 3;
    auto structural_order = timed([&] {
        auto ordered = norder(nall(shuffled));
        for (nidx_t i = 0; i < n; i += 97) checksum += ordered[i];
    });
    vector<nidx_t> run_values(n);
    for (nidx_t i = 0; i < n; ++i) run_values[i] = i / 4 % 137;
    auto run_projection = timed([&] {
        for (nidx_t repeat = 0; repeat < 20; ++repeat) {
            auto runs = nruns(nall(run_values));
            checksum += runs.len();
            for (nidx_t i = 0; i < runs.len(); i += 31)
                checksum += runs.key(i).second - runs.key(i).first + runs[i][0];
        }
    });

    nfhq<nidx_t> sequence;
    sequence.reserve(n);
    nidx_t root = -1;
    auto fhq_build = timed([&] { root = sequence.build(nall(values)); });
    auto fhq_transactions = timed([&] {
        uint64_t state = 1;
        for (nidx_t i = 0; i < n; ++i) {
            state = state * 6364136223846793005ULL + 1;
            nidx_t cut = nidx_t(state % (n + 1));
            auto [left, right] = sequence.split(root, cut);
            root = sequence.merge(left, right);
        }
        for (nidx_t i = 0; i < 2000; ++i)
            checksum += sequence[sequence.kth(root, i * 97 % n)].value;
    });

    unique_ptr<nbag<nidx_t>> ordered;
    auto bag_build = timed([&] {
        ordered = make_unique<nbag<nidx_t>>();
        ordered->reserve(n + n / 8);
        for (nidx_t value : shuffled) ordered->insert(value);
    });
    auto bag_work = timed([&] {
        uint64_t state = 5;
        for (nidx_t i = 0; i < 500000; ++i) {
            state = state * 2862933555777941757ULL + 3037000493ULL;
            nidx_t key = nidx_t(state % (n + n / 10));
            checksum += ordered->lower_bound(key) + ordered->upper_bound(key);
            nidx_t position = nidx_t((state >> 32) % n);
            if (!(i & 31)) {
                nidx_t value = ordered->erase_at(position);
                ordered->insert(value);
            } else {
                checksum += (*ordered)[position];
            }
        }
    });
    if (ordered->len() != n || ordered->front() != 0 || ordered->back() != n - 1) return 4;

    vector<long long> numbers(n);
    iota(numbers.begin(), numbers.end(), 1);
    long long seg_build = 0, seg_work = 0;
    unique_ptr<nseg<long long>> fixed;
    seg_build = timed([&] { fixed = make_unique<nseg<long long>>(nall(numbers)); });
    seg_work = timed([&] {
        uint64_t state = 7;
        for (nidx_t i = 0; i < 500000; ++i) {
            state = state * 2862933555777941757ULL + 3037000493ULL;
            nidx_t left = nidx_t(state % n), width = nidx_t((state >> 32) % 1000);
            nidx_t right = min(n, left + width);
            checksum += uint64_t(fixed->fold(left, right));
            if (!(i & 31)) fixed->set(left, numbers[left]);
        }
    });

    unique_ptr<nwavelet<nidx_t>> wavelet;
    auto wavelet_build = timed([&] { wavelet = make_unique<nwavelet<nidx_t>>(nall(values)); });
    auto wavelet_work = timed([&] {
        uint64_t state = 11;
        for (nidx_t i = 0; i < n; ++i) {
            state = state * 2862933555777941757ULL + 3037000493ULL;
            nidx_t left = nidx_t(state % n), right = min(n, left + 1 + nidx_t((state >> 32) % 10000));
            checksum += wavelet->kth(left, right, nidx_t(state % (right - left)));
            checksum += wavelet->less(left, right, nidx_t(state % (n + 1)));
        }
    });

    unique_ptr<nlct<long long>> paths;
    auto lct_build = timed([&] {
        paths = make_unique<nlct<long long>>(nall(numbers));
        for (nidx_t vertex = 1; vertex < n; ++vertex) paths->link(vertex - 1, vertex);
    });
    auto lct_work = timed([&] {
        uint64_t state = 13;
        for (nidx_t i = 0; i < n; ++i) {
            state = state * 6364136223846793005ULL + 1442695040888963407ULL;
            nidx_t a = nidx_t(state % n), b = nidx_t((state >> 32) % n);
            checksum += paths->fold(a, b);
            checksum += paths->path_size(a, b);
        }
    });

    vector<vector<nidx_t>> adjacency(n);
    struct bench_edge { nidx_t from, to; };
    vector<bench_edge> records;
    records.reserve(4 * n);
    for (nidx_t vertex = 0; vertex < n; ++vertex) {
        adjacency[vertex].push_back((vertex + 1) % n);
        adjacency[vertex].push_back((vertex * 37LL + 11) % n);
        adjacency[vertex].push_back((vertex * 97LL + 23) % n);
        adjacency[vertex].push_back((vertex * 193LL + 47) % n);
        for (nidx_t to : adjacency[vertex]) records.push_back({vertex, to});
    }
    vector<nidx_t> direct_distance;
    auto direct_bfs = timed([&] {
        direct_distance.assign(n, -1);
        vector<nidx_t> queue{0};
        direct_distance[0] = 0;
        for (nidx_t at = 0; at < nidx_t(queue.size()); ++at)
            for (nidx_t to : adjacency[queue[at]])
                if (direct_distance[to] < 0)
                    direct_distance[to] = direct_distance[queue[at]] + 1, queue.push_back(to);
    });
    auto graph = ngraph{nrange(n), [&](nidx_t vertex) -> auto& { return adjacency[vertex]; }};
    vector<nidx_t> projected_distance;
    auto projected_bfs = timed([&] { projected_distance = nbfs(graph, 0); });
    auto csr_start = clock_type::now();
    auto csr = nmake_csr(n, nall(records), [](bench_edge edge) { return edge.from; },
                         [](bench_edge edge) { return edge.to; });
    auto csr_build = chrono::duration_cast<chrono::milliseconds>(clock_type::now() - csr_start).count();
    vector<nidx_t> csr_distance;
    auto csr_bfs = timed([&] { csr_distance = nbfs(csr, 0); });
    checksum += accumulate(projected_distance.begin(), projected_distance.end(), uint64_t{});
    if (direct_distance != projected_distance || direct_distance != csr_distance) return 2;

    long long root_time = 0;
    auto rooted = timed([&] {
        auto forest = nroot(graph, nrange(n));
        checksum += forest.order().len();
        root_time = static_cast<long long>(forest.child_position.capacity() * sizeof(nidx_t) +
                                           forest.child_offset.capacity() * sizeof(nidx_t));
    });

    nsparse_seg<long long> sparse(0, 1LL << 40);
    sparse.reserve(1700000);
    nidx_t version = -1;
    auto sparse_time = timed([&] {
        uint64_t state = 19;
        for (nidx_t i = 0; i < 40000; ++i) {
            state = state * 6364136223846793005ULL + 1442695040888963407ULL;
            version = sparse.set_copy(version,
                                      static_cast<long long>(state & ((1ULL << 40) - 1)), i);
        }
        for (nidx_t i = 0; i < 40000; ++i)
            checksum += sparse.get(version, (1LL * i * 1000003) & ((1LL << 40) - 1));
    });

    cout << "n=" << n << " edges=" << 4LL * n << '\n';
    cout << "direct_sort_ms=" << direct_sort << " projected_sort_ms=" << projected_sort
         << " structural_order_ms=" << structural_order << " runs_20x_ms=" << run_projection << '\n';
    cout << "fhq_node_bytes=" << sizeof(nfhq<nidx_t>::node)
         << " build_ms=" << fhq_build << " split_merge_ms=" << fhq_transactions << '\n';
    cout << "bag_nodes=" << ordered->nodes() << " build_ms=" << bag_build
         << " query_mutate_ms=" << bag_work << '\n';
    cout << "fixed_seg_build_ms=" << seg_build << " workload_ms=" << seg_work << '\n';
    cout << "wavelet_build_ms=" << wavelet_build << " workload_ms=" << wavelet_work << '\n';
    cout << "lct_node_bytes=" << sizeof(nlct<long long>::node)
         << " build_ms=" << lct_build << " workload_ms=" << lct_work << '\n';
    cout << "direct_bfs_ms=" << direct_bfs << " graph_port_bfs_ms=" << projected_bfs << '\n';
    cout << "csr_build_ms=" << csr_build << " csr_bfs_ms=" << csr_bfs << '\n';
    cout << "root_projection_ms=" << rooted << " child_csr_capacity_bytes=" << root_time << '\n';
    cout << "sparse_node_bytes=" << sizeof(nsparse_seg<long long>::node)
         << " nodes=" << sparse.nodes() << " workload_ms=" << sparse_time << '\n';
    cout << "peak_rss_kib=" << peak_rss_kib() << " checksum=" << checksum << '\n';
}
