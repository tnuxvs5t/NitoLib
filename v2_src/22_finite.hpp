class npartition {
    nvector<int> classes_;
    int count_ = 0;

  public:
    npartition() = default;
    explicit npartition(nvector<int> labels) : classes_(move(labels)) {
        unordered_map<int, int> dense;
        dense.reserve(size_t(classes_.len()));
        for (int i = 0; i < classes_.len(); ++i) {
            auto [position, inserted] = dense.emplace(classes_[i], count_);
            if (inserted)
                ++count_;
            classes_[i] = position->second;
        }
    }

    int len() const noexcept { return classes_.len(); }
    int classes() const noexcept { return count_; }
    bool empty() const noexcept { return classes_.empty(); }

    int classof(int element, int fallback = npos) const {
        return 0 <= element && element < len() ? classes_[element] : fallback;
    }
    int operator[](int element) const {
        npre(0 <= element && element < len());
        return classes_[element];
    }
    bool same(int a, int b) const {
        npre(0 <= a && a < len() && 0 <= b && b < len());
        return classes_[a] == classes_[b];
    }

    nvector<nvector<int>> groups() const {
        nvector<nvector<int>> result(count_);
        for (int element = 0; element < len(); ++element)
            result[classes_[element]].push(element);
        return result;
    }

    friend bool operator==(const npartition&, const npartition&) = default;
};

using npart = npartition;

class nperm {
    nvector<int> image_;

    void validate() const {
        nvector<unsigned char> seen(len());
        for (int i = 0; i < len(); ++i) {
            npre(0 <= image_[i] && image_[i] < len());
            npre(!seen[image_[i]]);
            seen[image_[i]] = 1;
        }
    }

  public:
    nperm() = default;
    explicit nperm(int n) : image_(n) {
        for (int i = 0; i < n; ++i)
            image_[i] = i;
    }
    explicit nperm(nvector<int> image) : image_(move(image)) { validate(); }
    nperm(initializer_list<int> image) : image_(image) { validate(); }

    int len() const noexcept { return image_.len(); }
    bool empty() const noexcept { return image_.empty(); }
    int operator()(int value) const {
        npre(0 <= value && value < len());
        return image_[value];
    }
    int operator[](int value) const { return (*this)(value); }

    nperm inverse() const {
        nvector<int> result(len());
        for (int i = 0; i < len(); ++i)
            result[image_[i]] = i;
        return nperm(move(result));
    }
    friend nperm operator~(const nperm& permutation) { return permutation.inverse(); }

    // Composition follows function notation: (f*g)(x) = f(g(x)).
    friend nperm operator*(const nperm& f, const nperm& g) {
        npre(f.len() == g.len());
        nvector<int> result(f.len());
        for (int i = 0; i < f.len(); ++i)
            result[i] = f(g(i));
        return nperm(move(result));
    }

    nperm pow(int64_t exponent) const {
        nperm base = exponent < 0 ? inverse() : *this;
        uint64_t remaining = exponent < 0 ? uint64_t{} - uint64_t(exponent) : uint64_t(exponent);
        nperm result(len());
        while (remaining) {
            if (remaining & 1)
                result = base * result;
            remaining >>= 1;
            if (remaining)
                base = base * base;
        }
        return result;
    }

    npartition cycles() const {
        nvector<int> labels(len(), npos);
        int label = 0;
        for (int first = 0; first < len(); ++first)
            if (labels[first] == npos) {
                for (int value = first; labels[value] == npos; value = image_[value])
                    labels[value] = label;
                ++label;
            }
        return npartition(move(labels));
    }

    template <nindexed A> auto pull(const A& source) const {
        using T = nindex_value_t<const A>;
        npre(nlen(source) == len());
        nvector<T> result(len());
        for (int i = 0; i < len(); ++i)
            result[i] = source[image_[i]];
        return result;
    }
    template <nindexed A> auto push(const A& source) const {
        using T = nindex_value_t<const A>;
        npre(nlen(source) == len());
        nvector<T> result(len());
        for (int i = 0; i < len(); ++i)
            result[image_[i]] = source[i];
        return result;
    }

    friend bool operator==(const nperm&, const nperm&) = default;
};
