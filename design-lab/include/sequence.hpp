#pragma once
#include <algorithm>
#include <functional>
#include <numeric>
#include <span>
#include <utility>
#include <vector>

// Positions fit int. Source supports size() and O(1) operator[]. Equal keys keep
// their original order; projection/comparison are pure. O(n log n), O(n) storage.
template <class R, class Less = std::less<>, class Key = std::identity>
std::vector<int> argsort(const R& values, Less less = {}, Key key = {}) {
    std::vector<int> order(values.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        auto&& x = std::invoke(key, values[a]);
        auto&& y = std::invoke(key, values[b]);
        if (std::invoke(less, x, y)) return true;
        if (std::invoke(less, y, x)) return false;
        return a < b;
    });
    return order;
}

// Owns neither array. Their storage must outlive this object and remain stable.
// Repeated positions alias. No iterator category or constant-time inverse promise.
template <class T>
struct gathered {
    std::span<T> values;
    std::span<const int> positions;
    int size() const { return int(positions.size()); }
    T& operator[](int i) const { return values[positions[i]]; }
};

template <class T, std::size_t N, std::size_t M>
auto gather(std::span<T, N> values, std::span<const int, M> positions) {
    return gathered<T>{values, positions};
}

// Snapshot of maximal adjacent-equal runs. No retained source, callbacks after
// return, inverse propagation, or shared ownership. O(n) plus equality cost.
template <class R, class Equal = std::equal_to<>>
std::vector<std::pair<int, int>> run_bounds(const R& values, Equal equal = {}) {
    std::vector<std::pair<int, int>> bounds;
    int n = int(values.size());
    for (int left = 0; left < n;) {
        int right = left + 1;
        while (right < n && std::invoke(equal, values[right - 1], values[right])) ++right;
        bounds.emplace_back(left, right);
        left = right;
    }
    return bounds;
}
