#include "common.hpp"

struct Item {
    int color, count;
    friend bool operator==(const Item&, const Item&) = default;
};

int main() {
    auto descending = nrange(100, 90, -2);
    ntest(descending.position(100) == 0 && descending.position(92) == 4);
    ntest(descending.position(99) == npos && descending.position(90) == npos &&
          descending.position(102) == npos);

    auto aligned = nfunc_bind(nrange(0, 5), nrange(100, 90, -2));
    ntest(aligned.len() == 5 && aligned.key(3) == 3 && aligned[3] == 94);
    ntest(aligned(0) == 100 && aligned(4) == 92);

    nvector<int> anchors{40, 10, 30};
    nvector<string> labels{"forty", "ten", "thirty"};
    auto table = nfunc_bind(anchors, labels);
    ntest(table(10) == "ten" && table(30) == "thirty");
    anchors[1] = 99;
    ntest(table.key(1) == 10 && table(10) == "ten");
    anchors[1] = 10;
    table(40) = "40";
    ntest(labels[0] == "40");

    auto reanchored = nanchors(labels, anchors);
    ntest(reanchored.key(1) == 10 && reanchored[1] == "ten" &&
          reanchored(30) == "thirty");

    auto schedule = nfunc_bind(nrange(3, -1, -1), nrange(10, 18, 2));
    ntest(schedule(3) == 10 && schedule(2) == 12 && schedule(0) == 16);

    nvector<long long> dp{5, 8, 13};
    int base_calls = 0, alternative_calls = 0;
    auto state = nfunc_ref(nrange(dp.len()), [&](int index) -> long long& {
        ++base_calls;
        npre(0 <= index && index < dp.len());
        return dp[index];
    });
    auto safe = nbranch_value(
        state,
        [](int index) { return index == -1; },
        [&](int) {
            ++alternative_calls;
            return 0LL;
        });
    ntest(safe(-1) == 0 && base_calls == 0 && alternative_calls == 1);
    ntest(safe(2) == 13 && base_calls == 1 && alternative_calls == 1);

    long long sentinel = -7;
    auto writable = nbranch_ref(
        state,
        [](int index) { return index == -1; },
        [&](int) -> long long& { return sentinel; });
    writable(-1) = 21;
    writable(1) = 34;
    ntest(sentinel == 21 && dp[1] == 34);

    nvector<int> colors{1, 1, 2, 2, 5, 4, 4};
    auto runs = nruns(colors);
    ntest(runs.len() == 4);
    ntest(ncollect(nkeys(runs)) == nvector<int>({0, 2, 4, 5}));
    ntest(ncollect(runs[0]) == nvector<int>({1, 1}));
    ntest(ncollect(runs(5)) == nvector<int>({4, 4}));

    auto items = nmap_values(runs, [](auto run) {
        return Item{run[0], run.len()};
    });
    auto chains = nruns(items, [](const Item& left, const Item& right) {
        return abs(left.color - right.color) == 1;
    });
    auto nested = ncollect(nmap_values(chains, [](auto chain) {
        return ncollect(chain);
    }));
    ntest((nested == nvector<nvector<Item>>{
                         {{1, 2}, {2, 2}},
                         {{5, 1}, {4, 2}},
                     }));

    auto detached = nruns(nvector<int>{7, 7, 8})[0];
    ntest(ncollect(detached) == nvector<int>({7, 7}));
}
