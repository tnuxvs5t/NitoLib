#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    vector<nidx_t> sample{2, 3, 5};
    auto move_prefix = nprefix(
        nall(sample), make_unique<long long>(7),
        [salt = make_unique<nidx_t>(11)](const unique_ptr<long long>& accumulated, nidx_t value) {
            return make_unique<long long>(*accumulated * 3 + value + *salt);
        });
    CHECK(move_prefix.size() == 4);
    CHECK(*move_prefix[0] == 7 && *move_prefix[1] == 34);
    CHECK(*move_prefix[2] == 116 && *move_prefix[3] == 364);

    auto move_suffix = nsuffix(
        nall(sample), make_unique<string>("I"),
        [](nidx_t value, const unique_ptr<string>& accumulated) {
            return make_unique<string>(to_string(value) + "[" + *accumulated + "]");
        });
    CHECK(move_suffix.size() == 4);
    CHECK(*move_suffix[0] == "2[3[5[I]]]" && *move_suffix[3] == "I");

    mt19937_64 rng(0x5CA11ULL);
    for (nidx_t round = 0; round < 20000; ++round) {
        nidx_t n = nidx_t(rng() % 81);
        vector<nidx_t> source(n);
        for (nidx_t& value : source) value = nidx_t(rng() % 2001) - 1000;

        long long identity = static_cast<long long>(rng() % 2001) - 1000;
        auto prefix_sum = nprefix(nall(source), identity);
        vector<long long> prefix_brute{identity};
        for (nidx_t value : source) prefix_brute.push_back(prefix_brute.back() + value);
        CHECK(prefix_sum == prefix_brute);

        auto suffix_sum = nsuffix(nall(source), identity);
        vector<long long> suffix_brute(n + 1);
        suffix_brute[n] = identity;
        for (nidx_t i = n; i-- > 0;) suffix_brute[i] = source[i] + suffix_brute[i + 1];
        CHECK(suffix_sum == suffix_brute);

        uint64_t seed = rng();
        auto prefix_order = nprefix(
            nall(source), seed, [](const uint64_t& accumulated, nidx_t value) {
                return rotl(accumulated, 9) ^ uint64_t(uint32_t(value));
            });
        vector<uint64_t> prefix_order_brute{seed};
        for (nidx_t value : source)
            prefix_order_brute.push_back(rotl(prefix_order_brute.back(), 9) ^
                                         uint64_t(uint32_t(value)));
        CHECK(prefix_order == prefix_order_brute);

        auto suffix_order = nsuffix(
            nall(source), seed, [](nidx_t value, const uint64_t& accumulated) {
                return uint64_t(uint32_t(value)) ^ rotl(accumulated, 13);
            });
        vector<uint64_t> suffix_order_brute(n + 1);
        suffix_order_brute[n] = seed;
        for (nidx_t i = n; i-- > 0;)
            suffix_order_brute[i] = uint64_t(uint32_t(source[i])) ^
                                    rotl(suffix_order_brute[i + 1], 13);
        CHECK(suffix_order == suffix_order_brute);
    }
}
