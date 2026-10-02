#include "../include/sequence.hpp"
#include <iostream>

int main() {
    struct Record { int key; long long value; };
    std::vector<Record> rows{{4, 10}, {2, 3}, {4, 5}, {2, 7}, {9, 1}};
    auto order = argsort(rows, std::less<>{}, &Record::key);
    auto sorted = gather(std::span(rows), std::span<const int>(order));
    auto groups = run_bounds(sorted, [](const Record& a, const Record& b) {
        return a.key == b.key;
    });
    for (auto [left, right] : groups) {
        long long total = 0;
        for (int i = left; i < right; ++i) total += rows[order[i]].value;
        std::cout << rows[order[left]].key << ": " << total << '\n';
    }
}
