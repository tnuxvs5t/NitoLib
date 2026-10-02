#include "../include/sequence.hpp"
#include <cstdlib>
#include <iostream>
#include <map>
#include <random>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x << '\n'; std::abort(); } } while (false)

int main() {
    std::vector<int> empty;
    CHECK(argsort(empty).empty() && run_bounds(empty).empty());
    std::vector<int> one{7}, repeated{0, 0};
    auto aliases = gather(std::span(one), std::span<const int>(repeated));
    aliases[1] = 9;
    CHECK(one[0] == 9 && aliases[0] == 9);
    std::mt19937 rng(8301);
    for (int trial = 0; trial < 2500; ++trial) {
        struct Row { int key, payload; };
        int n = int(rng() % 90);
        std::vector<Row> rows(n);
        std::map<int, std::vector<int>> oracle;
        for (int i = 0; i < n; ++i) {
            rows[i] = {int(rng() % 13) - 6, i};
            oracle[rows[i].key].push_back(i);
        }
        auto order = argsort(rows, std::less<>{}, &Row::key);
        std::vector<int> expected;
        for (const auto& [key, ids] : oracle) {
            (void)key;
            expected.insert(expected.end(), ids.begin(), ids.end());
        }
        CHECK(order == expected);
        auto sorted = gather(std::span(rows), std::span<const int>(order));
        auto groups = run_bounds(sorted, [](const Row& a, const Row& b) { return a.key == b.key; });
        CHECK(groups.size() == oracle.size());
        int at = 0;
        for (const auto& [key, ids] : oracle) {
            auto [l, r] = groups[at++];
            CHECK(r - l == int(ids.size()));
            for (int i = l; i < r; ++i) CHECK(sorted[i].key == key && sorted[i].payload == ids[i - l]);
        }
        auto descending = argsort(rows, std::greater<>{}, &Row::key);
        for (int i = 1; i < n; ++i) CHECK(rows[descending[i - 1]].key >= rows[descending[i]].key);
    }
    std::cout << "sequence: 2500 random stable plans/groupings + alias/empty cases\n";
}
