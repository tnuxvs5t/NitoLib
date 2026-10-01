#include "../src-v4/hash.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct constant_hash {
    size_t operator()(nidx_t) const { return 0; }
};

int main() {
    nhash fixed(0x123456789abcdef0ULL);
    CHECK(fixed(pair{17, 23}) == fixed(pair{17, 23}));
    CHECK(fixed(pair{17, 23}) != fixed(pair{23, 17}));
    CHECK(fixed(tuple{17, 23, 31}) == fixed(tuple{17, 23, 31}));

    nidx_t first = 1, second = 2, third = 3;
    pair<nidx_t&, nidx_t&> inner{first, second};
    using nested_ref = pair<pair<nidx_t&, nidx_t&>&, nidx_t&>;
    using nested_value = pair<pair<nidx_t, nidx_t>, nidx_t>;
    static_assert(same_as<nowned_t<nested_ref>, nested_value>);
    nested_ref nested{inner, third};
    CHECK(fixed(nested) == fixed(nested));

    vector<nidx_t> keys{1};
    for (nidx_t i = 0; i < 2000; ++i) keys.push_back(i * 37 + 11);
    auto index = nmake_hash_inverse(nall(keys), nhash(7), equal_to<>{});
    CHECK(index.find(1) == 0 && index.find(2) == -1);
    for (nidx_t i = 0; i < 2000; ++i)
        CHECK(index.find(i * 37 + 11) == i + 1);

    vector<nidx_t> collisions(200);
    iota(collisions.begin(), collisions.end(), -100);
    auto collision_index = nmake_hash_inverse(
        nall(collisions), constant_hash{}, equal_to<>{}
    );
    for (nidx_t i = 0; i < nidx_t(collisions.size()); ++i)
        CHECK(collision_index.find(collisions[i]) == i);
    CHECK(collision_index.find(1000) == -1);

    vector<string> words{"north", "east", "south", "west"};
    auto inverted = ninvert(nall(words), nhash(11), equal_to<>{});
    CHECK(inverted.inverse(string_view("south")) == 2);
    CHECK(inverted[3] == "west");

    auto structural = ninvert(nrange(10, 20));
    static_assert(same_as<decltype(structural), decltype(nrange(10, 20))>);
    CHECK(structural.inverse(17) == 7);

    auto product = nproduct(nrange(2), nrange(3));
    auto owned = ninvert(product);
    CHECK(owned.inverse(pair{1, 2}) == 5);

    cout << "v4 hash: structural and static open-address inverse passed\n";
}
