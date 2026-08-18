#include "../src-v3/hash.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    nhash fixed(0x123456789abcdef0ULL);
    CHECK(fixed(pair{17, 23}) == fixed(pair{17, 23}));
    CHECK(fixed(tuple{17, 23, 31}) == fixed(tuple{17, 23, 31}));
    CHECK(fixed(pair{17, 23}) != fixed(pair{23, 17}));

    int first = 1, second = 2, third = 3;
    using nested_reference_key = pair<pair<int&, int&>&, int&>;
    using nested_owned_key = pair<pair<int, int>, int>;
    static_assert(same_as<nview_detail::owned_t<nested_reference_key>, nested_owned_key>);
    pair<int&, int&> inner{first, second};
    nested_reference_key nested{inner, third};
    CHECK(fixed(nested) == fixed(nested));

    vector<int> fixed_keys{1};
    for (int i = 0; i < 2000; ++i)
        fixed_keys.push_back(i * 37 + 11);
    auto index = nmake_hash_inverse(nall(fixed_keys), nhash(7), equal_to<>{});
    static_assert(decltype(index)::storage_slot_bytes() == 8);
    CHECK(index.find(1) == 0);
    CHECK(index.find(2) == -1);
    for (int i = 0; i < 2000; ++i)
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
        size_t operator()(int) const { return 0; }
    };
    vector<int> collisions(200);
    iota(collisions.begin(), collisions.end(), -100);
    auto collision_index = nmake_hash_inverse(
        nall(collisions), constant_hash{}, equal_to<>{}
    );
    for (int i = 0; i < int(collisions.size()); ++i)
        CHECK(collision_index.find(collisions[i]) == i);
    CHECK(collision_index.find(1000) == -1);

    mt19937_64 rng(0xA51CEDULL);
    for (int round = 0; round < 300; ++round) {
        int n = int(rng() % 80);
        vector<int> keys(n);
        for (int i = 0; i < n; ++i)
            keys[i] = int(rng() % 1000000);
        sort(keys.begin(), keys.end());
        keys.erase(unique(keys.begin(), keys.end()), keys.end());

        auto table = nmake_hash_inverse(
            nall(keys), nhash(uint64_t(round) + 1), equal_to<>{}
        );
        for (int x = -20; x < 1000020; x += 7919) {
            auto it = find(keys.begin(), keys.end(), x);
            auto got = table.find(x);
            CHECK((got != -1) == (it != keys.end()));
            if (got != -1)
                CHECK(got == int(it - keys.begin()));
        }
    }
}
