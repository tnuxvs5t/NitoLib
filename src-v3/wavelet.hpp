#pragma once
#include "view.hpp"

/*
Static wavelet matrix over the rank-compression of T.  T only needs copying, == and a
strict ordering accepted by sort/lower_bound.  Construction is O(n log n+n log sigma),
storage is O(n log sigma), and access/rank/less/kth are O(log sigma).  All query ranges
are valid [left,right); kth uses 0 <= order < right-left.
*/
template <class T>
struct nwavelet {
    nidx_t length = 0, levels = 0;
    vector<T> alphabet;
    vector<nidx_t> zero;
    vector<vector<nidx_t>> ones;

    nwavelet() = default;

    template <class V>
    explicit nwavelet(V source) : length(source.len()) {
        alphabet.reserve(length);
        for (nidx_t i = 0; i < length; ++i) alphabet.push_back(source[i]);
        sort(alphabet.begin(), alphabet.end());
        alphabet.erase(unique(alphabet.begin(), alphabet.end()), alphabet.end());
        if (alphabet.size() > 1) levels = bit_width(nuidx_t(alphabet.size() - 1));
        vector<nidx_t> current(length), low, high;
        for (nidx_t i = 0; i < length; ++i)
            current[i] = nidx_t(lower_bound(alphabet.begin(), alphabet.end(), source[i]) -
                             alphabet.begin());
        zero.resize(levels);
        ones.assign(levels, vector<nidx_t>(length + 1));
        low.reserve(length);
        high.reserve(length);
        for (nidx_t level = 0; level < levels; ++level) {
            nidx_t shift = levels - 1 - level;
            low.clear();
            high.clear();
            for (nidx_t i = 0; i < length; ++i) {
                bool bit = current[i] >> shift & 1;
                ones[level][i + 1] = ones[level][i] + bit;
                (bit ? high : low).push_back(current[i]);
            }
            zero[level] = nidx_t(low.size());
            current.clear();
            current.insert(current.end(), low.begin(), low.end());
            current.insert(current.end(), high.begin(), high.end());
        }
    }

    nidx_t len() const { return length; }
    bool empty() const { return !length; }

  private:
    pair<nidx_t, nidx_t> descend(nidx_t level, nidx_t left, nidx_t right, bool bit) const {
        nidx_t left_ones = ones[level][left], right_ones = ones[level][right];
        if (bit) return {zero[level] + left_ones, zero[level] + right_ones};
        return {left - left_ones, right - right_ones};
    }

    nidx_t frequency_rank(nidx_t left, nidx_t right, nidx_t rank) const {
        if (rank < 0 || rank >= nidx_t(alphabet.size())) return 0;
        for (nidx_t level = 0; level < levels; ++level) {
            bool bit = rank >> (levels - 1 - level) & 1;
            tie(left, right) = descend(level, left, right, bit);
        }
        return right - left;
    }

    nidx_t less_rank(nidx_t left, nidx_t right, nidx_t rank) const {
        if (rank <= 0) return 0;
        if (rank >= nidx_t(alphabet.size())) return right - left;
        nidx_t result = 0;
        for (nidx_t level = 0; level < levels; ++level) {
            nidx_t left_ones = ones[level][left], right_ones = ones[level][right];
            nidx_t zeros = (right - left) - (right_ones - left_ones);
            bool bit = rank >> (levels - 1 - level) & 1;
            if (bit) result += zeros;
            tie(left, right) = descend(level, left, right, bit);
        }
        return result;
    }

  public:
    T access(nidx_t position) const {
        nidx_t rank = 0;
        for (nidx_t level = 0; level < levels; ++level) {
            bool bit = ones[level][position + 1] != ones[level][position];
            if (bit) rank |= nidx_t(1) << (levels - 1 - level);
            tie(position, ignore) = descend(level, position, position + 1, bit);
        }
        return alphabet[rank];
    }

    T kth(nidx_t left, nidx_t right, nidx_t order) const {
        nidx_t rank = 0;
        for (nidx_t level = 0; level < levels; ++level) {
            nidx_t left_ones = ones[level][left], right_ones = ones[level][right];
            nidx_t zeros = (right - left) - (right_ones - left_ones);
            bool bit = order >= zeros;
            if (bit) order -= zeros, rank |= nidx_t(1) << (levels - 1 - level);
            tie(left, right) = descend(level, left, right, bit);
        }
        return alphabet[rank];
    }

    nidx_t count(nidx_t left, nidx_t right, const T& value) const {
        auto it = lower_bound(alphabet.begin(), alphabet.end(), value);
        if (it == alphabet.end() || !(*it == value)) return 0;
        return frequency_rank(left, right, nidx_t(it - alphabet.begin()));
    }

    nidx_t less(nidx_t left, nidx_t right, const T& upper) const {
        nidx_t rank = nidx_t(lower_bound(alphabet.begin(), alphabet.end(), upper) - alphabet.begin());
        return less_rank(left, right, rank);
    }

    nidx_t count(nidx_t left, nidx_t right, const T& lower, const T& upper) const {
        return less(left, right, upper) - less(left, right, lower);
    }

    optional<T> next(nidx_t left, nidx_t right, const T& lower) const {
        nidx_t order = less(left, right, lower);
        if (order == right - left) return nullopt;
        return kth(left, right, order);
    }

    optional<T> previous(nidx_t left, nidx_t right, const T& upper) const {
        nidx_t order = less(left, right, upper);
        if (!order) return nullopt;
        return kth(left, right, order - 1);
    }
};

template <class V>
nwavelet(V) -> nwavelet<remove_cvref_t<decltype(declval<V>()[0])>>;
