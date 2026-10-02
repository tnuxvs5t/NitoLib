#pragma once
#include <algorithm>
#include <bit>
#include <cassert>
#include <functional>
#include <utility>
#include <vector>

// Monoid supplies value_type, id(), and join(a,b). Associative, ordered; no
// commutativity requirement. n and padded 2*base fit int. R has size() and [].
// Owns values. Build O(n); set/fold/boundary O(log n), times monoid/predicate cost.
template <class Monoid>
struct segtree {
    using T = typename Monoid::value_type;
    int n, base;
    [[no_unique_address]] Monoid op;
    std::vector<T> data;

    template <class R>
    explicit segtree(const R& source, Monoid operation = {})
        : n(int(source.size())), base(int(std::bit_ceil(unsigned(std::max(1, n))))),
          op(std::move(operation)), data(2 * base, op.id()) {
        for (int i = 0; i < n; ++i) data[base + i] = source[i];
        for (int i = base - 1; i; --i) data[i] = op.join(data[2 * i], data[2 * i + 1]);
    }

    void set(int i, T value) {
        data[i += base] = std::move(value);
        while (i >>= 1) data[i] = op.join(data[2 * i], data[2 * i + 1]);
    }
    T fold(int left, int right) const {
        T a = op.id(), b = op.id();
        for (left += base, right += base; left < right; left >>= 1, right >>= 1) {
            if (left & 1) a = op.join(a, data[left++]);
            if (right & 1) b = op.join(data[--right], b);
        }
        return op.join(a, b);
    }
    const T& all() const { return data[1]; }

    // predicate(id()) is true, then stays false under right extension.
    template <class Predicate>
    int max_right(int left, Predicate predicate) const {
        T aggregate = op.id();
        assert(std::invoke(predicate, aggregate));
        if (left == n) return n;
        int node = left + base;
        do {
            while (!(node & 1)) node >>= 1;
            T candidate = op.join(aggregate, data[node]);
            if (!std::invoke(predicate, candidate)) {
                while (node < base) {
                    node *= 2;
                    candidate = op.join(aggregate, data[node]);
                    if (std::invoke(predicate, candidate)) {
                        aggregate = std::move(candidate);
                        ++node;
                    }
                }
                return std::min(n, node - base);
            }
            aggregate = std::move(candidate);
            ++node;
        } while ((node & -node) != node);
        return n;
    }

    // predicate(id()) is true, then stays false under left extension.
    template <class Predicate>
    int min_left(int right, Predicate predicate) const {
        T aggregate = op.id();
        assert(std::invoke(predicate, aggregate));
        if (!right) return 0;
        int node = right + base;
        do {
            --node;
            while (node > 1 && (node & 1)) node >>= 1;
            T candidate = op.join(data[node], aggregate);
            if (!std::invoke(predicate, candidate)) {
                while (node < base) {
                    node = 2 * node + 1;
                    candidate = op.join(data[node], aggregate);
                    if (std::invoke(predicate, candidate)) {
                        aggregate = std::move(candidate);
                        --node;
                    }
                }
                return std::max(0, node + 1 - base);
            }
            aggregate = std::move(candidate);
        } while ((node & -node) != node);
        return 0;
    }
};
