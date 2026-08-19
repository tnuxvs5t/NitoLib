#pragma once
#include "view.hpp"

template <class V>
vector<nidx_t> nprefix_function(V sequence) {
    vector<nidx_t> prefix(sequence.len());
    for (nidx_t i = 1; i < sequence.len(); ++i) {
        nidx_t border = prefix[i - 1];
        while (border && !(sequence[i] == sequence[border])) border = prefix[border - 1];
        if (sequence[i] == sequence[border]) ++border;
        prefix[i] = border;
    }
    return prefix;
}

template <class V>
vector<nidx_t> nz(V sequence) {
    nidx_t n = sequence.len();
    vector<nidx_t> z(n);
    if (n) z[0] = n;
    for (nidx_t i = 1, left = 0, right = 0; i < n; ++i) {
        if (i < right) z[i] = min(right - i, z[i - left]);
        while (i + z[i] < n && sequence[z[i]] == sequence[i + z[i]]) ++z[i];
        if (right < i + z[i]) left = i, right = i + z[i];
    }
    return z;
}

/* Empty pattern occurs at every boundary [0,text.len()]. */
template <class T, class P>
vector<nidx_t> nkmp(T text, P pattern) {
    vector<nidx_t> answer;
    if (!pattern.len()) {
        answer.resize(text.len() + 1);
        iota(answer.begin(), answer.end(), 0);
        return answer;
    }
    auto prefix = nprefix_function(pattern);
    for (nidx_t i = 0, matched = 0; i < text.len(); ++i) {
        while (matched && !(text[i] == pattern[matched])) matched = prefix[matched - 1];
        if (text[i] == pattern[matched]) ++matched;
        if (matched == pattern.len()) answer.push_back(i + 1 - matched), matched = prefix[matched - 1];
    }
    return answer;
}

struct npalindrome_radii {
    vector<nidx_t> odd, even;
};

/* odd[i] includes center i; even[i] is centered between i-1 and i. */
template <class V>
npalindrome_radii nmanacher(V sequence) {
    nidx_t n = sequence.len();
    vector<nidx_t> odd(n), even(n);
    for (nidx_t i = 0, left = 0, right = -1; i < n; ++i) {
        nidx_t radius = i > right ? 1 : min(odd[left + right - i], right - i + 1);
        while (0 <= i - radius && i + radius < n &&
               sequence[i - radius] == sequence[i + radius]) ++radius;
        odd[i] = radius;
        if (right < i + radius - 1) left = i - radius + 1, right = i + radius - 1;
    }
    for (nidx_t i = 0, left = 0, right = -1; i < n; ++i) {
        nidx_t radius = i > right ? 0 : min(even[left + right - i + 1], right - i + 1);
        while (0 <= i - radius - 1 && i + radius < n &&
               sequence[i - radius - 1] == sequence[i + radius]) ++radius;
        even[i] = radius;
        if (right < i + radius - 1) left = i - radius, right = i + radius - 1;
    }
    return {move(odd), move(even)};
}

/* Generic comparable alphabet; counting by ranks makes each doubling round O(n). */
template <class V>
vector<nidx_t> nsuffix_array(V sequence) {
    nidx_t n = sequence.len();
    vector<nidx_t> suffix(n), rank(n), next_rank(n), candidate;
    iota(suffix.begin(), suffix.end(), 0);
    sort(suffix.begin(), suffix.end(), [&](nidx_t a, nidx_t b) { return sequence[a] < sequence[b]; });
    for (nidx_t i = 1; i < n; ++i)
        rank[suffix[i]] = rank[suffix[i - 1]] +
                          (sequence[suffix[i - 1]] < sequence[suffix[i]]);
    for (nidx_t width = 1; width < n; width <<= 1) {
        candidate.clear();
        candidate.reserve(n);
        for (nidx_t position = max(nidx_t(0), n - width); position < n; ++position)
            candidate.push_back(position);
        for (nidx_t position : suffix)
            if (position >= width) candidate.push_back(position - width);
        nidx_t classes = rank[suffix.back()] + 1;
        vector<nidx_t> count(classes), start(classes);
        for (nidx_t position : candidate) ++count[rank[position]];
        partial_sum(count.begin(), count.end() - 1, start.begin() + 1);
        for (nidx_t position : candidate) suffix[start[rank[position]]++] = position;
        next_rank[suffix[0]] = 0;
        for (nidx_t i = 1; i < n; ++i) {
            nidx_t a = suffix[i - 1], b = suffix[i];
            pair left{rank[a], a + width < n ? rank[a + width] : -1};
            pair right{rank[b], b + width < n ? rank[b + width] : -1};
            next_rank[b] = next_rank[a] + (left != right);
        }
        rank.swap(next_rank);
        if (rank[suffix.back()] + 1 == n) break;
    }
    return suffix;
}

template <class V>
vector<nidx_t> nlcp(V sequence, const vector<nidx_t>& suffix) {
    nidx_t n = sequence.len(), height = 0;
    vector<nidx_t> rank(n), lcp(max(nidx_t(0), n - 1));
    for (nidx_t i = 0; i < n; ++i) rank[suffix[i]] = i;
    for (nidx_t start = 0; start < n; ++start) {
        nidx_t position = rank[start];
        if (position + 1 == n) { height = 0; continue; }
        nidx_t other = suffix[position + 1];
        while (start + height < n && other + height < n &&
               sequence[start + height] == sequence[other + height]) ++height;
        lcp[position] = height;
        if (height) --height;
    }
    return lcp;
}
