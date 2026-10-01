#include "../src-v4/reftree.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using U = uint64_t;

struct count_ops {
    U count(const auto& q, nidx_t root, nidx_t height) const {
        return q[root].value << (height - q[root].height);
    }
    U join(const auto& q, nidx_t height, nidx_t left, nidx_t right) const {
        return count(q, left, height - 1) + count(q, right, height - 1);
    }
};

using bits = nreftree<U, count_ops>;

struct text_ops {
    string join(const auto& q, nidx_t, nidx_t left, nidx_t right) const {
        return q[left].value + q[right].value;
    }
};

void verify(const bits& q, nidx_t root, nidx_t height, const vector<U>& expected) {
    auto count = [&](nidx_t handle, nidx_t h) { return q.ops.count(q, handle, h); };
    auto has = [&](nidx_t handle, nidx_t h) { return count(handle, h) != 0; };
    nidx_t before = q.nodes();
    CHECK(count(root, height) == accumulate(expected.begin(), expected.end(), U{}));
    for (U x = 0; x < expected.size(); ++x) {
        CHECK(q[q.leaf_at(root, x)].value == expected[x]);
        U first = x;
        while (first < expected.size() && !expected[first]) ++first;
        CHECK(q.find_first(root, height, x, has) == first);
    }
    vector<U> selected;
    for (U x = 0; x < expected.size(); ++x)
        if (expected[x]) selected.push_back(x);
    for (U k = 0; k < selected.size(); ++k)
        CHECK(q.kth(root, height, k, count) == selected[k]);
    CHECK(q.find_first(root, height, U(1) << height, has) == (U(1) << height));
    CHECK(q.nodes() == before);
}

int main() {
    bits q;
    nidx_t one = q.leaf(1), zero = q.leaf(0);
    nidx_t odd = q.join(1, zero, one), even = q.join(1, one, zero);
    CHECK(odd != even && q[q.join(1, zero, one)].value == 1);
    CHECK(q.split(odd, 60) == pair(odd, odd) && q.join(60, odd, odd) == odd);

    for (nidx_t height = 0; height <= 3; ++height) {
        U n = U(1) << height;
        for (U mask = 0; mask < (U(1) << n); ++mask) {
            vector<U> expected(n);
            nidx_t root = zero;
            for (U x = 0; x < n; ++x) {
                expected[x] = (mask >> x) & 1;
                if (expected[x]) root = q.set(root, height, x, one);
            }
            verify(q, root, height, expected);
            for (nidx_t width = 0; width <= height; ++width) {
                U block_width = U(1) << width;
                for (U x = 0; x < n; x += block_width) {
                    auto part = q.block(root, x, width);
                    verify(q, part, width,
                           vector<U>(expected.begin() + x,
                                     expected.begin() + x + block_width));
                }
            }
        }
    }

    mt19937_64 rng(0x52454654524545ULL);
    for (nidx_t height = 1; height <= 8; ++height) {
        U n = U(1) << height;
        vector<nidx_t> roots{zero, one};
        vector<vector<U>> values{vector<U>(n), vector<U>(n, 1)};
        for (nidx_t round = 0; round < 700; ++round) {
            size_t base = rng() % roots.size(), source = rng() % roots.size();
            nidx_t width_height = nidx_t(rng() % U(height + 1));
            nidx_t source_height = nidx_t(rng() % U(width_height + 1));
            U width = U(1) << width_height, source_width = U(1) << source_height;
            U x = rng() % (n / width) * width;
            U y = rng() % (n / source_width) * source_width;
            nidx_t part = q.block(roots[source], y, source_height);
            auto expected = values[base];
            for (U j = 0; j < width; ++j)
                expected[x + j] = values[source][y + j % source_width];
            nidx_t before = q.nodes();
            nidx_t next = q.paste(roots[base], height, x, width_height, part);
            CHECK(q.nodes() - before <= height - width_height);
            verify(q, next, height, expected);
            verify(q, roots[base], height, values[base]);
            roots.push_back(next);
            values.push_back(move(expected));
        }
    }

    nreftree<string, text_ops> text;
    nidx_t a = text.leaf("a"), b = text.leaf("b");
    nidx_t ab = text.join(1, a, b);
    CHECK(text[ab].value == "ab" && text[text.join(1, b, a)].value == "ba");

    nreftree<> bare;
    nidx_t left = bare.leaf(), right = bare.leaf();
    nidx_t pair_root = bare.join(1, left, right);
    CHECK(bare.leaf_at(pair_root, 6) == left && bare.leaf_at(pair_root, 7) == right);
    nidx_t before = bare.nodes();
    bare.reserve(before + 100);
    CHECK(bare.nodes() == before && bare[pair_root].left == left);

    cout << "v4 reftree: persistent patterns, aligned paste, search/select and order passed\n";
}
