#include "../src-v3/func.hpp"

using clock_type = chrono::steady_clock;

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

template <class K, class Make, class Lookup>
void run_case(string name, const vector<K>& keys, Make make, Lookup lookup) {
    auto build_start = clock_type::now();
    auto table = make();
    auto build_ms = chrono::duration_cast<chrono::milliseconds>(
        clock_type::now() - build_start
    ).count();

    uint64_t checksum = 0, state = 0x123456789abcdef0ULL;
    auto lookup_start = clock_type::now();
    for (nidx_t repeat = 0; repeat < 10 * nidx_t(keys.size()); ++repeat) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        checksum += uint64_t(lookup(table, keys[state % keys.size()]));
    }
    auto lookup_ms = chrono::duration_cast<chrono::milliseconds>(
        clock_type::now() - lookup_start
    ).count();
    cout << name << " build_ms=" << build_ms << " lookup_ms=" << lookup_ms
         << " peak_rss_kib=" << peak_rss_kib() << " checksum=" << checksum << '\n';
}

int main(int argc, char** argv) {
    if (argc != 2 || (string(argv[1]) != "node" && string(argv[1]) != "flat"))
        return 2;
    bool flat = string(argv[1]) == "flat";
    constexpr uint64_t salt = 0x3141592653589793ULL;

    using tuple_key = tuple<nidx_t, nidx_t, nidx_t>;
    cout << "layout index_key=" << nhash_inverse<nidx_t>::storage_key_bytes()
         << " index_slot=" << nhash_inverse<nidx_t>::storage_slot_bytes()
         << " tuple_key=" << nhash_inverse<tuple_key>::storage_key_bytes()
         << " tuple_slot=" << nhash_inverse<tuple_key>::storage_slot_bytes() << '\n';

    vector<nidx_t> integers(200000);
    iota(integers.begin(), integers.end(), 0);
    vector<tuple<nidx_t, nidx_t, nidx_t>> tuples;
    tuples.reserve(100000);
    for (nidx_t i = 0; i < 100000; ++i)
        tuples.emplace_back(i / 10000, i / 100 % 100, i % 100);

    auto run_int = [&] {
        if (flat) {
            run_case<nidx_t>("flat<nidx_t>", integers,
                          [&] {
                              return nmake_hash_inverse(
                                  nall(integers), nhash(salt), equal_to<>{}
                              );
                          },
                          [](auto& table, nidx_t key) { return table.find(key); });
        } else {
            run_case<nidx_t>("node<nidx_t>", integers,
                          [&] {
                              unordered_map<nidx_t, nidx_t, nhash> table(0, nhash(salt));
                              table.max_load_factor(0.7f);
                              table.reserve(integers.size());
                              for (nidx_t i = 0; i < nidx_t(integers.size()); ++i)
                                  table.emplace(integers[i], i);
                              return table;
                          },
                          [](auto& table, nidx_t key) { return table.find(key)->second; });
        }
    };

    auto run_tuple = [&] {
        if (flat) {
            run_case<tuple<nidx_t, nidx_t, nidx_t>>("flat<tuple>", tuples,
                          [&] {
                              return nmake_hash_inverse(
                                  nall(tuples), nhash(salt), equal_to<>{}
                              );
                          },
                          [](auto& table, const auto& key) { return table.find(key); });
        } else {
            run_case<tuple<nidx_t, nidx_t, nidx_t>>("node<tuple>", tuples,
                          [&] {
                              using key_type = tuple_key;
                              unordered_map<key_type, nidx_t, nhash> table(0, nhash(salt));
                              table.max_load_factor(0.7f);
                              table.reserve(tuples.size());
                              for (nidx_t i = 0; i < nidx_t(tuples.size()); ++i)
                                  table.emplace(tuples[i], i);
                              return table;
                          },
                          [](auto& table, const auto& key) { return table.find(key)->second; });
        }
    };

    run_int();
    run_tuple();
}
