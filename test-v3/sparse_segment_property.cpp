#include "../src-v3/segment.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

constexpr long long mod = 1'000'000'007;
struct mod_add {
    long long id() const { return 0; }
    long long operator()(long long left, long long right) const {
        return (left + right) % mod;
    }
};

int main() {
    mt19937 rng(0xD15EA5E);
    constexpr nidx_t lo = -40, hi = 61, width = hi - lo;

    nsparse_seg<long long, mod_add> persistent(lo, hi);
    vector<nidx_t> versions{-1};
    vector<vector<long long>> reference(1, vector<long long>(width));
    for (nidx_t round = 0; round < 20000; ++round) {
        nidx_t source = nidx_t(rng() % versions.size());
        nidx_t operation = nidx_t(rng() % 3);
        if (operation < 2) {
            nidx_t position = lo + nidx_t(rng() % width);
            auto next = reference[source];
            nidx_t root;
            if (operation == 0) {
                long long value = rng() % 2001;
                root = persistent.set_copy(versions[source], position, value);
                next[position - lo] = value;
            } else {
                long long value = rng() % 101;
                root = persistent.combine_copy(versions[source], position, value);
                next[position - lo] = (next[position - lo] + value) % mod;
            }
            versions.push_back(root);
            reference.push_back(move(next));
        } else {
            nidx_t other = nidx_t(rng() % versions.size());
            nidx_t root = persistent.merge_copy(versions[source], versions[other]);
            vector<long long> next(width);
            for (nidx_t i = 0; i < width; ++i)
                next[i] = (reference[source][i] + reference[other][i]) % mod;
            versions.push_back(root);
            reference.push_back(move(next));
        }
        for (nidx_t check = 0; check < 3; ++check) {
            nidx_t nodes_before = persistent.nodes();
            nidx_t version = nidx_t(rng() % versions.size());
            nidx_t left = nidx_t(rng() % (width + 1));
            nidx_t right = left + nidx_t(rng() % (width - left + 1));
            long long expected = 0;
            for (nidx_t i = left; i < right; ++i)
                expected = (expected + reference[version][i]) % mod;
            CHECK(persistent.fold(versions[version], lo + left, lo + right) == expected);
            CHECK(persistent.nodes() == nodes_before);
        }
    }

    nsparse_seg<long long> destructive(lo, hi);
    vector<nidx_t> roots(12, -1);
    vector<vector<long long>> arrays(12, vector<long long>(width));
    for (nidx_t round = 0; round < 30000; ++round) {
        nidx_t operation = nidx_t(rng() % 3);
        nidx_t a = nidx_t(rng() % roots.size());
        if (operation < 2) {
            nidx_t position = lo + nidx_t(rng() % width);
            long long value = nidx_t(rng() % 101) - 50;
            if (operation == 0)
                roots[a] = destructive.set(roots[a], position, value),
                arrays[a][position - lo] = value;
            else
                roots[a] = destructive.combine(roots[a], position, value),
                arrays[a][position - lo] += value;
        } else {
            nidx_t b = nidx_t(rng() % roots.size());
            if (a == b) continue;
            roots[a] = destructive.merge(roots[a], roots[b]);
            roots[b] = -1;
            for (nidx_t i = 0; i < width; ++i) arrays[a][i] += exchange(arrays[b][i], 0);
        }
        nidx_t left = nidx_t(rng() % (width + 1));
        nidx_t right = left + nidx_t(rng() % (width - left + 1));
        nidx_t nodes_before = destructive.nodes();
        CHECK(destructive.fold(roots[a], lo + left, lo + right) ==
              accumulate(arrays[a].begin() + left, arrays[a].begin() + right, 0LL));
        CHECK(destructive.nodes() == nodes_before);
    }

    nidx_t copy = destructive.clone(roots[0]);
    auto before = arrays[0];
    copy = destructive.set(copy, lo, 1234567);
    CHECK(destructive.get(roots[0], lo) == before[0]);
    CHECK(destructive.get(copy, lo) == 1234567);

    nsparse_seg<string, concat> ordered(0, 4, concat{});
    nidx_t left = -1, right = -1;
    left = ordered.set(left, 0, "a");
    left = ordered.set(left, 1, "c");
    right = ordered.set(right, 0, "b");
    right = ordered.set(right, 1, "d");
    nidx_t joined = ordered.merge(left, right);
    CHECK(ordered.fold(joined, 0, 2) == "abcd");

    nsparse_seg<long long> extreme(LLONG_MIN, LLONG_MAX);
    nidx_t root = -1;
    root = extreme.set(root, LLONG_MIN, 7);
    root = extreme.set(root, LLONG_MAX - 1, 11);
    root = extreme.set(root, -1, 13);
    CHECK(extreme.get(root, LLONG_MIN) == 7);
    CHECK(extreme.get(root, LLONG_MAX - 1) == 11);
    CHECK(extreme.fold(root, LLONG_MIN, LLONG_MAX) == 31);
}
