#include "common.hpp"

int main() {
    constexpr int left_bound = -32, right_bound = 32, n = right_bound - left_bound;
    mt19937 rng(0x34d1a9U);

    ndynamic_seg<long long> point(left_bound, right_bound);
    ndynamic_addsum<long long> lazy(left_bound, right_bound);
    vector<long long> point_ref(n), lazy_ref(n);

    for (int step = 0; step < 5000; ++step) {
        int kind = int(rng() % 6);
        if (kind == 0) {
            int index = left_bound + int(rng() % n);
            long long value = int(rng() % 101) - 50;
            point.set(index, value);
            point_ref[size_t(index - left_bound)] = value;
        } else if (kind == 1) {
            int index = left_bound + int(rng() % n);
            long long delta = int(rng() % 31) - 15;
            point.combine(index, delta);
            point_ref[size_t(index - left_bound)] += delta;
        } else if (kind == 2) {
            int index = left_bound + int(rng() % n);
            long long value = int(rng() % 101) - 50;
            lazy.set(index, value);
            lazy_ref[size_t(index - left_bound)] = value;
        } else if (kind == 3) {
            int left = int(rng() % (n + 1)), right = int(rng() % (n + 1));
            if (left > right)
                swap(left, right);
            long long delta = int(rng() % 31) - 15;
            lazy.apply(left_bound + left, left_bound + right, delta);
            for (int i = left; i < right; ++i)
                lazy_ref[size_t(i)] += delta;
        } else {
            int left = int(rng() % (n + 1)), right = int(rng() % (n + 1));
            if (left > right)
                swap(left, right);
            long long point_sum = accumulate(point_ref.begin() + left, point_ref.begin() + right, 0LL);
            long long lazy_sum = accumulate(lazy_ref.begin() + left, lazy_ref.begin() + right, 0LL);
            ntest(point.fold(left_bound + left, left_bound + right) == point_sum);
            ntest(lazy.fold(left_bound + left, left_bound + right) == lazy_sum);
        }
    }
}
