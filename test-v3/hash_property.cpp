#include "../src-v3/hash.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    nhash fixed(0x123456789abcdef0ULL);
    CHECK(fixed(pair{17, 23}) == fixed(pair{17, 23}));
    CHECK(fixed(tuple{17, 23, 31}) == fixed(tuple{17, 23, 31}));
    CHECK(fixed(pair{17, 23}) != fixed(pair{23, 17}));

    nidx_t first = 1, second = 2, third = 3;
    using nested_reference_key = pair<pair<nidx_t&, nidx_t&>&, nidx_t&>;
    using nested_owned_key = pair<pair<nidx_t, nidx_t>, nidx_t>;
    static_assert(same_as<nview_detail::owned_t<nested_reference_key>, nested_owned_key>);
    pair<nidx_t&, nidx_t&> inner{first, second};
    nested_reference_key nested{inner, third};
    CHECK(fixed(nested) == fixed(nested));

    vector<nidx_t> fixed_keys{1};
    for (nidx_t i = 0; i < 2000; ++i)
        fixed_keys.push_back(i * 37 + 11);
    auto index = nmake_hash_inverse(nall(fixed_keys), nhash(7), equal_to<>{});
    static_assert(decltype(index)::storage_slot_bytes() ==
                  (sizeof(nidx_t) == 4 ? 8 : 16));
    CHECK(index.find(1) == 0);
    CHECK(index.find(2) == -1);
    for (nidx_t i = 0; i < 2000; ++i)
        CHECK(index.find(i * 37 + 11) == i + 1);
    CHECK(index.find(-1) == -1);

    vector<string> words{"north", "east", "south", "west"};
    auto inverted_words = ninvert(nall(words), nhash(11), equal_to<>{});
    CHECK(inverted_words.inverse(string_view("south")) == 2);
    CHECK(inverted_words[3] == "west");

    auto structural = ninvert(nrange(10, 20));
    static_assert(same_as<decltype(structural), decltype(nrange(10, 20))>);
    CHECK(structural.inverse(17) == 7);

    struct constant_hash {
        size_t operator()(nidx_t) const { return 0; }
    };
    vector<nidx_t> collisions(200);
    iota(collisions.begin(), collisions.end(), -100);
    auto collision_index = nmake_hash_inverse(
        nall(collisions), constant_hash{}, equal_to<>{}
    );
    for (nidx_t i = 0; i < nidx_t(collisions.size()); ++i)
        CHECK(collision_index.find(collisions[i]) == i);
    CHECK(collision_index.find(1000) == -1);

    mt19937_64 rng(0xA51CEDULL);
    for (nidx_t round = 0; round < 300; ++round) {
        nidx_t n = nidx_t(rng() % 80);
        vector<nidx_t> keys(n);
        for (nidx_t i = 0; i < n; ++i)
            keys[i] = nidx_t(rng() % 1000000);
        sort(keys.begin(), keys.end());
        keys.erase(unique(keys.begin(), keys.end()), keys.end());

        auto table = nmake_hash_inverse(
            nall(keys), nhash(uint64_t(round) + 1), equal_to<>{}
        );
        for (nidx_t x = -20; x < 1000020; x += 7919) {
            auto it = find(keys.begin(), keys.end(), x);
            auto got = table.find(x);
            CHECK((got != -1) == (it != keys.end()));
            if (got != -1)
                CHECK(got == nidx_t(it - keys.begin()));
        }
    }
}
