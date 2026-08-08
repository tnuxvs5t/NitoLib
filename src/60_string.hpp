template <nindexed A> nvector<int> nprefix_function(const A& sequence) {
    int n = nlen(sequence);
    nvector<int> prefix(n, 0);
    for (int i = 1; i < n; ++i) {
        int border = prefix[i - 1];
        while (border && !(sequence[i] == sequence[border]))
            border = prefix[border - 1];
        if (sequence[i] == sequence[border])
            ++border;
        prefix[i] = border;
    }
    return prefix;
}

template <nindexed A> nvector<int> nz_function(const A& sequence) {
    int n = nlen(sequence);
    nvector<int> z(n, 0);
    for (int i = 1, left = 0, right = 0; i < n; ++i) {
        if (i < right)
            z[i] = min(right - i, z[i - left]);
        while (i + z[i] < n && sequence[z[i]] == sequence[i + z[i]])
            ++z[i];
        if (right < i + z[i]) {
            left = i;
            right = i + z[i];
        }
    }
    if (n)
        z[0] = n;
    return z;
}

template <nindexed Text, nindexed Pattern> nvector<int> nkmp_find(const Text& text, const Pattern& pattern) {
    int n = nlen(text), m = nlen(pattern);
    nvector<int> result;
    if (!m) {
        npre(n < INT_MAX);
        result.reserve(n + 1);
        for (int i = 0; i <= n; ++i)
            result.push(i);
        return result;
    }
    auto prefix = nprefix_function(pattern);
    int matched = 0;
    for (int i = 0; i < n; ++i) {
        while (matched && !(text[i] == pattern[matched]))
            matched = prefix[matched - 1];
        if (text[i] == pattern[matched])
            ++matched;
        if (matched == m) {
            result.push(i - m + 1);
            matched = prefix[matched - 1];
        }
    }
    return result;
}

/**
 * Manacher radius index for an immutable sequence.  Equality must be stable for the
 * sequence lifetime; intervals are half-open and queries are O(1) after O(n) build.
 */
class npalindrome_index {
    nvector<int> odd_, even_;

  public:
    npalindrome_index() = default;

    template <nindexed A> explicit npalindrome_index(const A& sequence)
        : odd_(nlen(sequence), 0), even_(nlen(sequence), 0) {
        int n = nlen(sequence);
        for (int i = 0, left = 0, right = -1; i < n; ++i) {
            int mirror = i > right ? 0 : int(1LL * left + right - i);
            int radius = i > right ? 1 : min(odd_[mirror], right - i + 1);
            while (0 <= i - radius && i + radius < n && sequence[i - radius] == sequence[i + radius])
                ++radius;
            odd_[i] = radius;
            if (right < i + radius - 1) {
                left = i - radius + 1;
                right = i + radius - 1;
            }
        }
        for (int i = 0, left = 0, right = -1; i < n; ++i) {
            int mirror = i > right ? 0 : int(1LL * left + right - i + 1);
            int radius = i > right ? 0 : min(even_[mirror], right - i + 1);
            while (0 <= i - radius - 1 && i + radius < n &&
                   sequence[i - radius - 1] == sequence[i + radius])
                ++radius;
            even_[i] = radius;
            if (right < i + radius - 1) {
                left = i - radius;
                right = i + radius - 1;
            }
        }
    }

    int len() const noexcept { return odd_.len(); }
    int odd_radius(int center) const {
        npre(0 <= center && center < len());
        return odd_[center];
    }
    int even_radius(int right_center) const {
        npre(0 <= right_center && right_center < len());
        return even_[right_center];
    }
    bool pal(int left, int right) const {
        npre(0 <= left && left <= right && right <= len());
        int length = right - left;
        if (!length)
            return true;
        int center = left + (right - left) / 2;
        return length & 1 ? odd_[center] >= length / 2 + 1 : even_[center] >= length / 2;
    }
};

template <nindexed A> auto nmanacher(const A& sequence) { return npalindrome_index(sequence); }

template <nindexed A> nvector<int> nprefix(const A& sequence) {
    return nprefix_function(sequence);
}

template <nindexed A> nvector<int> nzfunc(const A& sequence) { return nz_function(sequence); }

template <nindexed Text, nindexed Pattern>
nvector<int> nkmp(const Text& text, const Pattern& pattern) {
    return nkmp_find(text, pattern);
}

using nmanacher_result = npalindrome_index;

// Suffix-array construction requires a strict weak ordering on symbols and compares
// suffixes lexicographically.  The source sequence is copied; queries use [l,r).
template <nindexed A, class C = nless<>> nvector<int> nsuffix_array(const A& sequence, C compare = {}) {
    int n = nlen(sequence);
    nvector<int> suffix(n), rank(n), next_rank(n);
    for (int i = 0; i < n; ++i)
        suffix[i] = i;
    nsort(suffix, [&](int a, int b) { return compare(sequence[a], sequence[b]); });
    for (int i = 1; i < n; ++i)
        rank[suffix[i]] = rank[suffix[i - 1]] +
                          int(compare(sequence[suffix[i - 1]], sequence[suffix[i]]) ||
                              compare(sequence[suffix[i]], sequence[suffix[i - 1]]));

    for (int length = 1; length < n && rank[suffix[n - 1]] + 1 < n; length <<= 1) {
        nsort(suffix, [&](int a, int b) {
            if (rank[a] != rank[b])
                return rank[a] < rank[b];
            int rank_a = 1LL * a + length < n ? rank[a + length] : npos;
            int rank_b = 1LL * b + length < n ? rank[b + length] : npos;
            return rank_a < rank_b;
        });
        next_rank[suffix[0]] = 0;
        for (int i = 1; i < n; ++i) {
            int a = suffix[i - 1], b = suffix[i];
            pair<int, int> key_a{rank[a], 1LL * a + length < n ? rank[a + length] : npos};
            pair<int, int> key_b{rank[b], 1LL * b + length < n ? rank[b + length] : npos};
            next_rank[b] = next_rank[a] + int(key_a != key_b);
        }
        swap(rank, next_rank);
        if (length > n / 2)
            break;
    }
    return suffix;
}

template <nindexed A> nvector<int> nlcp_array(const A& sequence, const nvector<int>& suffix) {
    int n = nlen(sequence);
    npre(suffix.len() == n);
    nvector<int> rank(n), lcp(n, 0);
    nvector<unsigned char> seen(n, 0);
    for (int i = 0; i < n; ++i) {
        npre(0 <= suffix[i] && suffix[i] < n);
        npre(!seen[suffix[i]]);
        seen[suffix[i]] = 1;
        rank[suffix[i]] = i;
    }
    int common = 0;
    for (int start = 0; start < n; ++start) {
        int position = rank[start];
        if (!position)
            continue;
        int previous = suffix[position - 1];
        while (start + common < n && previous + common < n &&
               sequence[start + common] == sequence[previous + common])
            ++common;
        lcp[position] = common;
        if (common)
            --common;
    }
    return lcp;
}
