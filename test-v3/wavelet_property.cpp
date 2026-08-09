#include "../src-v3/wavelet.hpp"

template <class T>
void check(bool condition, const T& message) {
    if (!condition) throw runtime_error(message);
}

template <class T>
void verify(const vector<T>& source, mt19937_64& random) {
    auto copy = source;
    nwavelet wave(nall(copy));
    check(wave.len() == int(source.size()), "length");
    check(wave.empty() == source.empty(), "empty");
    for (int i = 0; i < int(source.size()); ++i)
        check(wave.access(i) == source[i], "access");
    if (source.empty()) return;

    for (int query = 0; query < 300; ++query) {
        int left = int(random() % source.size());
        int right = left + 1 + int(random() % (source.size() - left));
        vector<T> sorted(source.begin() + left, source.begin() + right);
        sort(sorted.begin(), sorted.end());
        int order = int(random() % sorted.size());
        check(wave.kth(left, right, order) == sorted[order], "kth");

        const T& value = source[random() % source.size()];
        int equal = int(count(sorted.begin(), sorted.end(), value));
        int below = int(lower_bound(sorted.begin(), sorted.end(), value) - sorted.begin());
        check(wave.count(left, right, value) == equal, "rank");
        check(wave.less(left, right, value) == below, "less");

        const T& other = source[random() % source.size()];
        T lower = min(value, other), upper = max(value, other);
        int inside = int(lower_bound(sorted.begin(), sorted.end(), upper) -
                         lower_bound(sorted.begin(), sorted.end(), lower));
        check(wave.count(left, right, lower, upper) == inside, "range count");

        auto next = lower_bound(sorted.begin(), sorted.end(), value);
        auto got_next = wave.next(left, right, value);
        check(bool(got_next) == (next != sorted.end()), "next presence");
        if (got_next) check(*got_next == *next, "next value");

        auto previous = lower_bound(sorted.begin(), sorted.end(), value);
        auto got_previous = wave.previous(left, right, value);
        check(bool(got_previous) == (previous != sorted.begin()), "previous presence");
        if (got_previous) check(*got_previous == *prev(previous), "previous value");
    }
}

int main() {
    mt19937_64 random(0xc001d00d5eedULL);
    verify(vector<long long>{}, random);
    verify(vector<long long>(40, -7), random);
    verify(vector<long long>{LLONG_MIN, 0, LLONG_MAX, LLONG_MIN, -1, 1}, random);
    verify(vector<string>{"", "z", "aa", "z", "a", "aba"}, random);

    for (int trial = 0; trial < 500; ++trial) {
        int n = int(random() % 50);
        vector<long long> source(n);
        for (long long& value : source) {
            uint64_t mode = random() % 20;
            if (mode == 0) value = LLONG_MIN;
            else if (mode == 1) value = LLONG_MAX;
            else value = static_cast<long long>(random() % 17) - 8;
        }
        verify(source, random);
    }
}
