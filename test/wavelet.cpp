#include "common.hpp"

int main() {
    nvector<int> values{INT_MIN, 5, -2, 5, 9, 0, INT_MAX, -2};
    nwavelet<int> wavelet(values);
    ntest(wavelet.len() == values.len());
    ntest(wavelet.kth(0, 8, 0) == INT_MIN);
    ntest(wavelet.kth(0, 8, 7) == INT_MAX);
    ntest(wavelet.kth(1, 6, 2) == 5);
    ntest(wavelet.count(0, 8, -2) == 2);
    ntest(wavelet.count_less(0, 8, 5) == 4);
    ntest(wavelet.count(0, 8, -2, 6) == 5);
    ntest(wavelet.predecessor(0, 8, 5).val() == 0);
    ntest(wavelet.successor(0, 8, 5).val() == 5);
    ntest(!wavelet.predecessor(0, 8, INT_MIN));
    ntest(wavelet.successor(0, 8, INT_MAX).val() == INT_MAX);

    nwavelet<long long> empty;
    ntest(empty.empty());
}
