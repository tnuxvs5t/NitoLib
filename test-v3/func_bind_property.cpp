#include "../src-v3/func.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct counted_key {
    int value;
    friend bool operator==(const counted_key&, const counted_key&) = default;
};

struct counted_hash {
    int* calls;
    size_t operator()(const counted_key& key) const {
        ++*calls;
        return nhash(17)(key.value);
    }
};

int main() {
    auto grid = nproduct(nrange(10, 13), nrange(-2, 2));
    auto structural = nfunc_bind(move(grid), nrange(12));
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j) {
            pair key{10 + i, -2 + j};
            CHECK(structural(key) == 4 * i + j);
            CHECK(structural(10 + i, -2 + j) == 4 * i + j);
        }

    auto cube = nfunc_bind(
        nproduct(nrange(5, 7), nrange(-1, 2), nrange(4)), nrange(24)
    );
    CHECK(cube(5, -1, 0) == 0);
    CHECK(cube(6, 1, 3) == 23);

    vector<string> names{"alice", "bob", "carol", "dave"};
    vector<int> score{3, 5, 8, 13};
    auto fallback = nfunc_bind(nall(names), nall(score));
    CHECK(fallback("alice") == 3);
    CHECK(fallback(string_view("carol")) == 8);
    fallback("dave") = 21;
    CHECK(score[3] == 21);

    int hash_calls = 0;
    vector<counted_key> custom_keys{{7}, {11}, {19}};
    auto custom = nfunc_bind(
        nall(custom_keys), nrange(3), counted_hash{&hash_calls}, equal_to<>{}
    );
    CHECK(hash_calls == 3);
    CHECK(custom(counted_key{11}) == 1);
    CHECK(hash_calls == 4);

    vector<int> explicit_keys{20, 10, 30};
    vector<int> explicit_values{2, 1, 3};
    array<int, 31> position{};
    for (int i = 0; i < int(explicit_keys.size()); ++i)
        position[explicit_keys[i]] = i;
    auto explicit_locator = nfunc_bind(
        nall(explicit_keys), nall(explicit_values),
        [&](int key) { return position[key]; }
    );
    CHECK(explicit_locator(30) == 3);

    mt19937 rng(0xF00B4A);
    for (int round = 0; round < 10000; ++round) {
        int n = int(rng() % 60);
        vector<int> keys(n), values(n);
        iota(keys.begin(), keys.end(), -1000);
        shuffle(keys.begin(), keys.end(), rng);
        for (int& value : values) value = int(rng());

        auto function = nfunc_bind(nall(keys), nall(values));
        for (int i = 0; i < n; ++i) {
            CHECK(function[i] == values[i]);
            CHECK(function(keys[i]) == values[i]);
        }
    }
}
