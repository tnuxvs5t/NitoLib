#include "nv4"
#include "nv4"

#ifdef NITORI_INDEX_64
static_assert(same_as<nidx_t, long long>);
#else
static_assert(same_as<nidx_t, int>);
#endif

int main() {
    vector<int> values{3, 1, 4, 1, 5};
    auto view = nall(values);
    if (view.len() != 5 || view[2] != 4) return 1;

    nwavelet wavelet(values);
    if (wavelet.kth(0, 5, 2) != 3 || wavelet.count(0, 5, 1) != 2) return 2;

    ntopk_merge<int, 2> merge;
    auto best = merge.id();
    for (int value : values) merge.update(best, value);
    if (best.len() != 2 || best[0] != 5 || best[1] != 4) return 3;

    auto fraction = nfrac<>(1, 2) + nfrac<>(1, 3);
    if (fraction.numerator() != 5 || fraction.denominator() != 6) return 4;
}
