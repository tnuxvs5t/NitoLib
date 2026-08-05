template <integral T>
    requires(!same_as<remove_cv_t<T>, bool>)
class nwavelet {
    using U = make_unsigned_t<T>;
    static constexpr int bits_ = numeric_limits<U>::digits;

    int size_ = 0;
    array<vector<int>, bits_> zero_prefix_;
    array<int, bits_> zero_count_{};

    static constexpr U encode(T value) {
        U encoded = U(value);
        if constexpr (is_signed_v<T>)
            encoded ^= U(1) << (bits_ - 1);
        return encoded;
    }

    static constexpr T decode(U encoded) {
        if constexpr (is_signed_v<T>) {
            encoded ^= U(1) << (bits_ - 1);
            return bit_cast<T>(encoded);
        } else {
            return encoded;
        }
    }

    static int checked_size(int n) {
        npre(n >= 0);
        return n;
    }

    pair<int, int> descend_zero(int level, int left, int right) const {
        return {zero_prefix_[level][left], zero_prefix_[level][right]};
    }

    pair<int, int> descend_one(int level, int left, int right) const {
        return {zero_count_[level] + left - zero_prefix_[level][left],
                zero_count_[level] + right - zero_prefix_[level][right]};
    }

  public:
    nwavelet() {
        for (auto& prefix : zero_prefix_)
            prefix.assign(1, 0);
    }

    template <nindexed A> explicit nwavelet(const A& source) : size_(checked_size(nlen(source))) {
        vector<U> current(size_), next(size_);
        for (int i = 0; i < size_; ++i)
            current[i] = encode(T(source[i]));

        for (int level = 0; level < bits_; ++level) {
            int bit = bits_ - 1 - level;
            auto& prefix = zero_prefix_[level];
            prefix.resize(size_t(size_) + 1);
            for (int i = 0; i < size_; ++i)
                prefix[i + 1] = prefix[i] + int(((current[i] >> bit) & U(1)) == 0);
            int zeros = zero_count_[level] = prefix[size_];
            int zero_position = 0, one_position = zeros;
            for (U value : current)
                (((value >> bit) & U(1)) ? next[one_position++] : next[zero_position++]) = value;
            current.swap(next);
        }
    }

    int len() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    T kth(int left, int right, int rank) const {
        npre(0 <= left && left <= right && right <= size_);
        npre(0 <= rank && rank < right - left);
        U value = 0;
        for (int level = 0; level < bits_; ++level) {
            int bit = bits_ - 1 - level;
            int zeros = zero_prefix_[level][right] - zero_prefix_[level][left];
            if (rank < zeros) {
                tie(left, right) = descend_zero(level, left, right);
            } else {
                rank -= zeros;
                value |= U(1) << bit;
                tie(left, right) = descend_one(level, left, right);
            }
        }
        return decode(value);
    }

    int count_less(int left, int right, T bound) const {
        npre(0 <= left && left <= right && right <= size_);
        U value = encode(bound);
        int result = 0;
        for (int level = 0; level < bits_; ++level) {
            int bit = bits_ - 1 - level;
            int zeros = zero_prefix_[level][right] - zero_prefix_[level][left];
            if ((value >> bit) & U(1)) {
                result += zeros;
                tie(left, right) = descend_one(level, left, right);
            } else {
                tie(left, right) = descend_zero(level, left, right);
            }
        }
        return result;
    }

    int count(int left, int right, T value) const {
        npre(0 <= left && left <= right && right <= size_);
        U encoded = encode(value);
        for (int level = 0; level < bits_; ++level) {
            int bit = bits_ - 1 - level;
            if ((encoded >> bit) & U(1))
                tie(left, right) = descend_one(level, left, right);
            else
                tie(left, right) = descend_zero(level, left, right);
        }
        return right - left;
    }

    int count(int left, int right, T low, T high) const {
        npre(!(high < low));
        return count_less(left, right, high) - count_less(left, right, low);
    }

    nmaybe<T> predecessor(int left, int right, T bound) const {
        int count = count_less(left, right, bound);
        return count ? nmaybe<T>(kth(left, right, count - 1)) : nmaybe<T>{};
    }

    nmaybe<T> successor(int left, int right, T bound) const {
        int count = count_less(left, right, bound);
        return count < right - left ? nmaybe<T>(kth(left, right, count)) : nmaybe<T>{};
    }
};
