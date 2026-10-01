#include "../src-v4/segment.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct immutable_value { const long long value; };
struct immutable_sum {
    immutable_value id() const { return {0}; }
    immutable_value operator()(const immutable_value& a, const immutable_value& b) const {
        return {a.value + b.value};
    }
};

int main() {
    nsparse_seg<immutable_value, immutable_sum> immutable(0, 8);
    nidx_t first = immutable.set_copy(-1, 2, {7});
    nidx_t second = immutable.combine_copy(first, 2, {11});
    nidx_t both = immutable.merge_copy(first, second);
    CHECK(immutable.fold(first).value == 7);
    CHECK(immutable.fold(second).value == 18 && immutable.fold(both).value == 25);

    constexpr nidx_t lo = -40, hi = 61, width = hi - lo;
    nsparse_seg<long long> persistent(lo, hi);
    vector<nidx_t> versions{-1};
    vector<vector<long long>> reference(1, vector<long long>(width));
    mt19937 rng(0xD15EA5E);
    for (int round = 0; round < 7000; ++round) {
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
                next[position - lo] += value;
            }
            versions.push_back(root);
            reference.push_back(move(next));
        } else {
            nidx_t other = nidx_t(rng() % versions.size());
            nidx_t root = persistent.merge_copy(versions[source], versions[other]);
            vector<long long> next(width);
            for (nidx_t i = 0; i < width; ++i)
                next[i] = reference[source][i] + reference[other][i];
            versions.push_back(root);
            reference.push_back(move(next));
        }
        for (int check = 0; check < 2; ++check) {
            nidx_t nodes_before = persistent.nodes();
            nidx_t version = nidx_t(rng() % versions.size());
            nidx_t left = nidx_t(rng() % (width + 1));
            nidx_t right = left + nidx_t(rng() % (width - left + 1));
            long long expected = accumulate(reference[version].begin() + left,
                                            reference[version].begin() + right, 0LL);
            CHECK(persistent.fold(versions[version], lo + left, lo + right) == expected);
            CHECK(persistent.nodes() == nodes_before);
        }
    }

    nsparse_seg<long long> destructive(lo, hi);
    vector<nidx_t> roots(8, -1);
    vector<vector<long long>> arrays(8, vector<long long>(width));
    for (int round = 0; round < 9000; ++round) {
        nidx_t action = nidx_t(rng() % 3), a = nidx_t(rng() % roots.size());
        if (action < 2) {
            nidx_t position = lo + nidx_t(rng() % width);
            long long value = nidx_t(rng() % 101) - 50;
            if (action == 0) {
                roots[a] = destructive.set(roots[a], position, value);
                arrays[a][position - lo] = value;
            } else {
                roots[a] = destructive.combine(roots[a], position, value);
                arrays[a][position - lo] += value;
            }
        } else {
            nidx_t b = nidx_t(rng() % roots.size());
            if (a == b) continue;
            roots[a] = destructive.merge(roots[a], roots[b]);
            roots[b] = -1;
            for (nidx_t i = 0; i < width; ++i)
                arrays[a][i] += exchange(arrays[b][i], 0LL);
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

    nsparse_seg<long long> extreme(LLONG_MIN, LLONG_MAX);
    nidx_t root = -1;
    root = extreme.set(root, LLONG_MIN, 7);
    root = extreme.set(root, LLONG_MAX - 1, 11);
    root = extreme.set(root, -1, 13);
    CHECK(extreme.get(root, LLONG_MIN) == 7);
    CHECK(extreme.get(root, LLONG_MAX - 1) == 11);
    CHECK(extreme.fold(root, LLONG_MIN, LLONG_MAX) == 31);

    cout << "v4 sparse segment: persistent/destructive roots and extreme coordinates passed\n";
}
