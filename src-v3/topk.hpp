#pragma once
#include "core.hpp"

/*
Fixed-capacity ordered summaries.  ntopk<T,K> owns at most K candidates in best-first
order; an empty summary is the identity.  The ordinary operation is an associative
Monoid for a strict weak ranking relation.  Equal candidates keep the left summary's
representative, so it is ordered (not commutative) unless Better supplies a deterministic
tie-break.  The keyed operation additionally needs a deterministic tie-break between
different keys; otherwise a key improving from a tie can make its order depend on the
merge tree.

update() means submitting another candidate.  It is not an arbitrary replacement or
erase operation: an omitted candidate cannot be reconstructed from a K-item summary.
T is default-constructible, copyable and movable in this compact array implementation.
K is compile-time and intentionally small.  merge costs O(K); the keyed variant costs
O(K^2) because it linearly checks the evidence key.  KeyOf(value) must be equality
comparable.  The keyed variant keeps the best candidate for each key before truncating.
States passed to an operation must use the same ranking/key semantics; keyed states
have distinct keys.  Policies stay unchanged and key equality is an equivalence relation.
update returns whether the candidate was retained.  Positions lie in [0,len()), front
requires nonempty, and K fits nidx_t.  Updates may move candidates between array slots;
do not retain a slot reference as a candidate identity.  Comparator/key calls are const.
*/
template <class T, size_t K>
struct ntopk {
private:
    array<T, K> values{};
    size_t size_ = 0;

    template <class, size_t, class> friend struct ntopk_merge;
    template <class, size_t, class, class> friend struct ntopk_by_merge;

public:
    constexpr nidx_t len() const { return nidx_t(size_); }
    constexpr bool empty() const { return size_ == 0; }
    constexpr const T& operator[](nidx_t position) const { return values[size_t(position)]; }
    constexpr const T& front() const { return values[0]; }
};

template <class T, size_t K, class Better = greater<>>
struct ntopk_merge {
    [[no_unique_address]] Better better;

    constexpr ntopk<T, K> id() const { return {}; }

    constexpr bool update(ntopk<T, K>& state, T candidate) const {
        if constexpr (K == 0) {
            return false;
        } else {
            size_t position = 0;
            while (position < state.size_ && !invoke(better, candidate, state.values[position]))
                ++position;
            if (position >= K) return false;
            if (state.size_ < K) ++state.size_;
            for (size_t i = state.size_ - 1; i > position; --i)
                state.values[i] = move(state.values[i - 1]);
            state.values[position] = move(candidate);
            return true;
        }
    }

    constexpr ntopk<T, K> single(T candidate) const {
        ntopk<T, K> result;
        update(result, move(candidate));
        return result;
    }

    constexpr ntopk<T, K> operator()(const ntopk<T, K>& left,
                                     const ntopk<T, K>& right) const {
        ntopk<T, K> result;
        size_t i = 0, j = 0;
        while (result.size_ < K && (i < left.size_ || j < right.size_)) {
            bool take_left = j == right.size_;
            if (i < left.size_ && j < right.size_)
                take_left = !invoke(better, right.values[j], left.values[i]);
            result.values[result.size_++] = take_left ? left.values[i++] : right.values[j++];
        }
        return result;
    }
};

template <class T, size_t K, class Better, class KeyOf>
struct ntopk_by_merge {
    [[no_unique_address]] Better better;
    [[no_unique_address]] KeyOf key_of;

    constexpr ntopk<T, K> id() const { return {}; }

    constexpr bool update(ntopk<T, K>& state, T candidate) const {
        if constexpr (K == 0) {
            return false;
        } else {
            for (size_t i = 0; i < state.size_; ++i) {
                if (!(invoke(key_of, candidate) == invoke(key_of, state.values[i]))) continue;
                if (!invoke(better, candidate, state.values[i])) return false;
                state.values[i] = move(candidate);
                while (i > 0 && invoke(better, state.values[i], state.values[i - 1])) {
                    swap(state.values[i], state.values[i - 1]);
                    --i;
                }
                return true;
            }
            return ntopk_merge<T, K, Better>{better}.update(state, move(candidate));
        }
    }

    constexpr ntopk<T, K> single(T candidate) const {
        ntopk<T, K> result;
        update(result, move(candidate));
        return result;
    }

    constexpr ntopk<T, K> operator()(const ntopk<T, K>& left,
                                     const ntopk<T, K>& right) const {
        ntopk<T, K> result;
        for (size_t i = 0; i < left.size_; ++i) update(result, left.values[i]);
        for (size_t i = 0; i < right.size_; ++i) update(result, right.values[i]);
        return result;
    }
};
