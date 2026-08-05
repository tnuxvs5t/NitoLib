#include "common.hpp"

int main() {
    mt19937 rng(20260805);

    for (int repetition = 0; repetition < 300; ++repetition) {
        int count = int(rng() % 40);
        int first = int(rng() % 101) - 50;
        int step = int(rng() % 9) - 4;
        if (step == 0)
            step = 1;
        int last = first + step * count;
        auto range = nrange(first, last, step);
        ntest(range.len() == count);
        for (int index = 0; index < count; ++index)
            ntest(range.position(range[index]) == index);
        ntest(range.position(last) == npos);
    }

    for (int repetition = 0; repetition < 150; ++repetition) {
        int n = 1 + int(rng() % 50);
        nvector<int> order(n), keys(n), values(n);
        for (int i = 0; i < n; ++i)
            order[i] = i;
        shuffle(order.data(), order.data() + n, rng);
        for (int i = 0; i < n; ++i) {
            keys[i] = order[i] * 17 + 3;
            values[i] = int(rng());
        }

        auto function = nfunc(keys, values);
        for (int i = 0; i < n; ++i) {
            ntest(function.key(i) == keys[i] && function[i] == values[i]);
            ntest(function(keys[i]) == values[i]);
        }

        nvector<int> colors(n);
        for (int i = 0; i < n; ++i)
            colors[i] = int(rng() % 6);
        nvector<int> starts;
        for (int i = 0; i < n; ++i)
            if (i == 0 || colors[i - 1] != colors[i])
                starts.push(i);

        auto runs = nruns(colors);
        ntest(ncollect(nkeys(runs)) == starts);
        for (int run = 0; run < starts.len(); ++run) {
            int left = starts[run];
            int right = run + 1 < starts.len() ? starts[run + 1] : n;
            ntest(ncollect(runs[run]) == ncollect(nsub(colors, left, right)));
            ntest(ncollect(runs(left)) == ncollect(nsub(colors, left, right)));
        }
    }

    int forbidden_calls = 0;
    auto base = nfunc(nrange(8), [&](int index) {
        ++forbidden_calls;
        npre(index >= 0);
        return index * index;
    });
    auto safe = nbranch(base, [](int index) { return index < 0; }, -1);
    for (int index = -20; index < 0; ++index)
        ntest(safe(index) == -1);
    ntest(forbidden_calls == 0);
}
