#include "../src-v3/func.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct hash_key {
    uint64_t group, item;
};

struct key_hash {
    long long* calls;

    size_t operator()(const hash_key& key) const {
        ++*calls;
        uint64_t x = key.group + 0x9e3779b97f4a7c15ULL * (key.item + 1);
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return size_t(x ^ (x >> 31));
    }
};

struct key_equal {
    long long* calls;

    bool operator()(const hash_key& a, const hash_key& b) const {
        ++*calls;
        return a.group == b.group && a.item == b.item;
    }
};

int main() {
    mt19937_64 rng(0xA11C0A5ULL);
    for (nidx_t round = 0; round < 5000; ++round) {
        nidx_t n = nidx_t(rng() % 97);
        vector<hash_key> keys(n);
        vector<long long> values(n);
        uint64_t group = rng();
        for (nidx_t i = 0; i < n; ++i) {
            keys[i] = {group, uint64_t(i)};
        }
        shuffle(keys.begin(), keys.end(), rng);
        for (nidx_t i = 0; i < n; ++i)
            values[i] = static_cast<long long>(keys[i].item * 17 + 3);

        long long hash_calls = 0, equal_calls = 0;
        auto function = nfunc_bind(nall(keys), nall(values),
                                   key_hash{&hash_calls}, key_equal{&equal_calls});
        CHECK(function.len() == n);
        for (nidx_t i = 0; i < n; ++i) {
            hash_key copied = keys[i];
            CHECK(function.key(i).group == keys[i].group);
            CHECK(function[i] == values[i]);
            CHECK(function(copied) == values[i]);
        }

        if (n) {
            nidx_t p = nidx_t(rng() % n);
            function(keys[p]) += 11;
            CHECK(values[p] == static_cast<long long>(keys[p].item * 17 + 14));
        }
    }

    vector<hash_key> keys(1 << 14);
    vector<nidx_t> values(keys.size());
    for (nidx_t i = 0; i < nidx_t(keys.size()); ++i) {
        keys[i] = {0x123456789abcdef0ULL, uint64_t(i)};
        values[i] = i * 3;
    }
    long long hash_calls = 0, equal_calls = 0;
    auto large = nfunc_bind(nall(keys), nall(values),
                            key_hash{&hash_calls}, key_equal{&equal_calls});
    hash_calls = equal_calls = 0;
    hash_key copied = keys.back();
    CHECK(large(copied) == values.back());
    CHECK(hash_calls < 128 && equal_calls < 128);

    vector<hash_key> owned_keys{{1, 4}, {1, 9}};
    auto owned_values = nview{
        2, [data = make_unique<array<nidx_t, 2>>(array<nidx_t, 2>{40, 90})](nidx_t i) -> nidx_t& {
            return (*data)[i];
        }};
    auto move_only = nfunc_bind(nall(owned_keys), move(owned_values),
                                key_hash{&hash_calls}, key_equal{&equal_calls});
    static_assert(!copy_constructible<decltype(move_only)>);
    CHECK(move_only(owned_keys[1]) == 90);
    move_only[0] = 44;
    CHECK(move_only(owned_keys[0]) == 44);
}
