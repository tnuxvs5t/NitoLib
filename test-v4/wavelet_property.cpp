#include "../src-v4/view.hpp"
#include "../src-v4/wavelet.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

template <class T>
void verify_wavelet(const vector<T>& source, mt19937_64& rng) {
    vector<T> copy = source;
    nwavelet wave(nall(copy));
    CHECK(wave.len() == nidx_t(source.size()) && wave.empty() == source.empty());
    for (nidx_t i = 0; i < nidx_t(source.size()); ++i) CHECK(wave.access(i) == source[i]);
    if (source.empty()) return;
    for (nidx_t round = 0; round < 350; ++round) {
        nidx_t left = nidx_t(rng() % source.size());
        nidx_t right = left + 1 + nidx_t(rng() % (source.size() - left));
        vector<T> sorted(source.begin() + left, source.begin() + right);
        sort(sorted.begin(), sorted.end());
        nidx_t order = nidx_t(rng() % sorted.size());
        T value = source[rng() % source.size()];
        T other = source[rng() % source.size()];
        T lower = min(value, other), upper = max(value, other);
        CHECK(wave.kth(left, right, order) == sorted[order]);
        CHECK(wave.count(left, right, value) ==
              nidx_t(count(sorted.begin(), sorted.end(), value)));
        CHECK(wave.less(left, right, value) ==
              nidx_t(lower_bound(sorted.begin(), sorted.end(), value) - sorted.begin()));
        CHECK(wave.count(left, right, lower, upper) ==
              nidx_t(lower_bound(sorted.begin(), sorted.end(), upper) -
                     lower_bound(sorted.begin(), sorted.end(), lower)));
        auto next = lower_bound(sorted.begin(), sorted.end(), value);
        auto previous = lower_bound(sorted.begin(), sorted.end(), value);
        CHECK(bool(wave.next(left, right, value)) == (next != sorted.end()));
        CHECK(bool(wave.previous(left, right, value)) != (previous == sorted.begin()));
        if (next != sorted.end()) CHECK(*wave.next(left, right, value) == *next);
        if (previous != sorted.begin()) CHECK(*wave.previous(left, right, value) == *prev(previous));
    }
}

int main() {
    mt19937_64 rng(0xc001d00d5eedULL);
    verify_wavelet(vector<long long>{}, rng);
    verify_wavelet(vector<long long>(40, -7), rng);
    verify_wavelet(vector<long long>{LLONG_MIN, 0, LLONG_MAX, LLONG_MIN, -1, 1}, rng);
    verify_wavelet(vector<string>{"", "z", "aa", "z", "a", "aba"}, rng);
    for (nidx_t round = 0; round < 500; ++round) {
        nidx_t n = nidx_t(rng() % 50);
        vector<long long> source(n);
        for (auto& value : source) value = nidx_t(rng() % 17) - 8;
        verify_wavelet(source, rng);
    }
    cout << "v4 wavelet: access, order statistics and range ranks passed\n";
}
