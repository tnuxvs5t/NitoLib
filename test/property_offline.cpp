#include "common.hpp"

int main() {
    mt19937 rng(22360679);
    constexpr int n = 250, query_count = 2000;
    nvector<int> values(n);
    for (int i = 0; i < n; ++i)
        values[i] = int(rng() % 40);

    nvector<ninterval_query> queries;
    nvector<int> expected(query_count), actual(query_count);
    for (int id = 0; id < query_count; ++id) {
        int left = int(rng() % (n + 1));
        int right = int(rng() % (n + 1));
        if (left > right)
            swap(left, right);
        queries.push(ninterval_query{left, right, id});
        array<bool, 40> seen{};
        for (int i = left; i < right; ++i)
            seen[values[i]] = true;
        expected[id] = int(count(seen.begin(), seen.end(), true));
    }

    array<int, 40> frequency{};
    int distinct = 0;
    auto add = [&](int index) {
        if (frequency[values[index]]++ == 0)
            ++distinct;
    };
    auto remove = [&](int index) {
        if (--frequency[values[index]] == 0)
            --distinct;
    };
    nrun_mo(queries, n, add, remove, [&](int id) { actual[id] = distinct; });
    ntest(actual == expected);
}
