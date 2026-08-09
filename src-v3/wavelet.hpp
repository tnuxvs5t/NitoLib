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
    int length = 0, levels = 0;
    vector<T> alphabet;
    vector<int> zero;
    vector<vector<int>> ones;

    nwavelet() = default;

    template <class V>
    explicit nwavelet(V source) : length(source.len()) {
        alphabet.reserve(length);
        for (int i = 0; i < length; ++i) alphabet.push_back(source[i]);
        sort(alphabet.begin(), alphabet.end());
        alphabet.erase(unique(alphabet.begin(), alphabet.end()), alphabet.end());
        if (alphabet.size() > 1) levels = bit_width(unsigned(alphabet.size() - 1));
        vector<int> current(length), low, high;
        for (int i = 0; i < length; ++i)
            current[i] = int(lower_bound(alphabet.begin(), alphabet.end(), source[i]) -
                             alphabet.begin());
        zero.resize(levels);
        ones.assign(levels, vector<int>(length + 1));
        low.reserve(length);
        high.reserve(length);
        for (int level = 0; level < levels; ++level) {
            int shift = levels - 1 - level;
            low.clear();
            high.clear();
            for (int i = 0; i < length; ++i) {
                bool bit = current[i] >> shift & 1;
                ones[level][i + 1] = ones[level][i] + bit;
                (bit ? high : low).push_back(current[i]);
            }
            zero[level] = int(low.size());
            current.clear();
            current.insert(current.end(), low.begin(), low.end());
            current.insert(current.end(), high.begin(), high.end());
        }
    }

    int len() const { return length; }
    bool empty() const { return !length; }

  private:
    pair<int, int> descend(int level, int left, int right, bool bit) const {
        int left_ones = ones[level][left], right_ones = ones[level][right];
        if (bit) return {zero[level] + left_ones, zero[level] + right_ones};
        return {left - left_ones, right - right_ones};
    }

    int frequency_rank(int left, int right, int rank) const {
        if (rank < 0 || rank >= int(alphabet.size())) return 0;
        for (int level = 0; level < levels; ++level) {
            bool bit = rank >> (levels - 1 - level) & 1;
            tie(left, right) = descend(level, left, right, bit);
        }
        return right - left;
    }

    int less_rank(int left, int right, int rank) const {
        if (rank <= 0) return 0;
        if (rank >= int(alphabet.size())) return right - left;
        int result = 0;
        for (int level = 0; level < levels; ++level) {
            int left_ones = ones[level][left], right_ones = ones[level][right];
            int zeros = (right - left) - (right_ones - left_ones);
            bool bit = rank >> (levels - 1 - level) & 1;
            if (bit) result += zeros;
            tie(left, right) = descend(level, left, right, bit);
        }
        return result;
    }

  public:
    T access(int position) const {
        int rank = 0;
        for (int level = 0; level < levels; ++level) {
            bool bit = ones[level][position + 1] != ones[level][position];
            if (bit) rank |= 1 << (levels - 1 - level);
            tie(position, ignore) = descend(level, position, position + 1, bit);
        }
        return alphabet[rank];
    }

    T kth(int left, int right, int order) const {
        int rank = 0;
        for (int level = 0; level < levels; ++level) {
            int left_ones = ones[level][left], right_ones = ones[level][right];
            int zeros = (right - left) - (right_ones - left_ones);
            bool bit = order >= zeros;
            if (bit) order -= zeros, rank |= 1 << (levels - 1 - level);
            tie(left, right) = descend(level, left, right, bit);
        }
        return alphabet[rank];
    }

    int count(int left, int right, const T& value) const {
        auto it = lower_bound(alphabet.begin(), alphabet.end(), value);
        if (it == alphabet.end() || !(*it == value)) return 0;
        return frequency_rank(left, right, int(it - alphabet.begin()));
    }

    int less(int left, int right, const T& upper) const {
        int rank = int(lower_bound(alphabet.begin(), alphabet.end(), upper) - alphabet.begin());
        return less_rank(left, right, rank);
    }

    int count(int left, int right, const T& lower, const T& upper) const {
        return less(left, right, upper) - less(left, right, lower);
    }

    optional<T> next(int left, int right, const T& lower) const {
        int order = less(left, right, lower);
        if (order == right - left) return nullopt;
        return kth(left, right, order);
    }

    optional<T> previous(int left, int right, const T& upper) const {
        int order = less(left, right, upper);
        if (!order) return nullopt;
        return kth(left, right, order - 1);
    }
};

template <class V>
nwavelet(V) -> nwavelet<remove_cvref_t<decltype(declval<V>()[0])>>;
