#include "common.hpp"

int main() {
    mt19937 rng(17320508);
    constexpr int n = 180;
    nvector<int> values(n);
    for (int i = 0; i < n; ++i)
        values[i] = int(rng() % 101) - 50;
    nwavelet<int> wavelet(values);

    for (int trial = 0; trial < 10000; ++trial) {
        int left = int(rng() % (n + 1));
        int right = int(rng() % (n + 1));
        if (left > right)
            swap(left, right);
        vector<int> sorted;
        for (int i = left; i < right; ++i)
            sorted.push_back(values[i]);
        sort(sorted.begin(), sorted.end());

        int bound = int(rng() % 121) - 60;
        int expected_less = int(lower_bound(sorted.begin(), sorted.end(), bound) - sorted.begin());
        ntest(wavelet.count_less(left, right, bound) == expected_less);
        ntest(wavelet.count(left, right, bound) == int(count(sorted.begin(), sorted.end(), bound)));

        int low = int(rng() % 121) - 60;
        int high = int(rng() % 121) - 60;
        if (low > high)
            swap(low, high);
        int expected_range = int(lower_bound(sorted.begin(), sorted.end(), high) -
                                 lower_bound(sorted.begin(), sorted.end(), low));
        ntest(wavelet.count(left, right, low, high) == expected_range);

        if (!sorted.empty()) {
            int rank = int(rng() % sorted.size());
            ntest(wavelet.kth(left, right, rank) == sorted[rank]);
            auto previous = wavelet.predecessor(left, right, bound);
            auto next = wavelet.successor(left, right, bound);
            if (expected_less)
                ntest(previous && previous.val() == sorted[expected_less - 1]);
            else
                ntest(!previous);
            if (expected_less < int(sorted.size()))
                ntest(next && next.val() == sorted[expected_less]);
            else
                ntest(!next);
        }
    }
}
