#include "../src-v3/topk.hpp"
#include "../src-v3/segment.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct candidate {
    nidx_t score = 0, key = 0, evidence = 0;
    friend bool operator==(const candidate&, const candidate&) = default;
};

struct better_score {
    bool operator()(const candidate& left, const candidate& right) const {
        return left.score > right.score;
    }
};

struct better_score_key {
    bool operator()(const candidate& left, const candidate& right) const {
        if (left.score != right.score) return left.score > right.score;
        if (left.key != right.key) return left.key < right.key;
        return left.evidence < right.evidence;
    }
};

template <size_t K>
vector<candidate> values(const ntopk<candidate, K>& state) {
    vector<candidate> result;
    for (nidx_t i = 0; i < state.len(); ++i) result.push_back(state[i]);
    return result;
}

template <size_t K>
ntopk<candidate, K> from(const vector<candidate>& source,
                          ntopk_merge<candidate, K, better_score> op) {
    auto result = op.id();
    for (candidate value : source) op.update(result, move(value));
    return result;
}

template <size_t K>
vector<candidate> oracle(vector<candidate> source, bool keyed) {
    if (keyed) {
        vector<candidate> unique;
        for (candidate value : source) {
            auto it = find_if(unique.begin(), unique.end(), [&](const candidate& old) {
                return old.key == value.key;
            });
            if (it == unique.end()) unique.push_back(move(value));
            else if (better_score_key{}(value, *it)) *it = move(value);
        }
        source = move(unique);
    }
    if (keyed) stable_sort(source.begin(), source.end(), better_score_key{});
    else stable_sort(source.begin(), source.end(), better_score{});
    if (source.size() > K) source.resize(K);
    return source;
}

template <size_t K>
void check_state(const ntopk<candidate, K>& state, const vector<candidate>& expected) {
    CHECK(values(state) == expected);
}

int main() {
    ntopk_merge<int, 0> zero;
    auto empty = zero.id();
    CHECK(!zero.update(empty, 7) && zero.single(7).empty());
    CHECK(zero(empty, empty).empty());
    ntopk_by_merge<int, 0, greater<>, identity> zero_keyed;
    CHECK(!zero_keyed.update(empty, 7) && zero_keyed.single(7).empty());
    CHECK(zero_keyed(empty, empty).empty());

    ntopk_merge<candidate, 1, better_score> best;
    auto witness = best.single({7, 1, 10});
    CHECK(!best.update(witness, {7, 2, 20}));
    CHECK(witness.front() == candidate(7, 1, 10));
    CHECK(best.update(witness, {8, 2, 20}));
    CHECK(witness.front() == candidate(8, 2, 20));
    ntopk_merge<int, 1, less<>> minimum;
    CHECK(minimum(minimum.single(INT_MAX), minimum.single(INT_MIN)).front() == INT_MIN);
    ntopk_merge<int, 2> largest;
    auto duplicates = largest(largest.single(10), largest.single(9));
    CHECK(largest(duplicates, duplicates)[1] == 10);

    using ordinary = ntopk_merge<candidate, 4, better_score>;
    ordinary op;
    CHECK(op.id().empty());
    auto one = op.single({7, 3, 10});
    CHECK(one.len() == 1 && one.front() == candidate(7, 3, 10));
    CHECK(op.update(one, candidate{9, 1, 11}) && one.front().score == 9);
    CHECK(op.update(one, candidate{8, 2, 12}));
    CHECK(op.update(one, candidate{6, 4, 13}));
    CHECK(!op.update(one, candidate{5, 5, 14}));
    CHECK(!op.update(one, candidate{1, 8, 15}));

    mt19937 rng(0x70F);
    for (nidx_t round = 0; round < 6000; ++round) {
        vector<candidate> a(rng() % 11), b(rng() % 11), c(rng() % 11);
        auto make = [&] (vector<candidate>& source) {
            for (candidate& value : source) {
                value.score = nidx_t(rng() % 9) - 4;
                value.key = nidx_t(rng() % 5);
                value.evidence = nidx_t(rng());
            }
        };
        make(a); make(b); make(c);
        auto sa = from<4>(a, op), sb = from<4>(b, op), sc = from<4>(c, op);
        check_state(sa, oracle<4>(a, false));
        check_state(op(sa, op.id()), values(sa));
        check_state(op(op.id(), sa), values(sa));
        vector<candidate> all = a;
        all.insert(all.end(), b.begin(), b.end());
        all.insert(all.end(), c.begin(), c.end());
        check_state(op(op(sa, sb), sc), oracle<4>(all, false));
        check_state(op(sa, op(sb, sc)), oracle<4>(all, false));
        check_state(op(op(sa, sb), sc), values(op(sa, op(sb, sc))));
        CHECK(op(op(sa, sb), sc).len() <= 4);
    }

    using key_function = decltype([](const candidate& value) { return value.key; });
    ntopk_by_merge<candidate, 4, better_score_key, key_function> keyed{
        better_score_key{}, key_function{}};
    auto distinct = keyed.single({10, 1, 1});
    CHECK(!keyed.update(distinct, {9, 1, 2}));
    CHECK(keyed.update(distinct, {8, 2, 3}));
    CHECK(keyed.update(distinct, {12, 2, 4}));
    CHECK(distinct.len() == 2 && distinct.front() == candidate(12, 2, 4));
    for (nidx_t round = 0; round < 6000; ++round) {
        vector<candidate> a(rng() % 11), b(rng() % 11), c(rng() % 11);
        auto make = [&] (vector<candidate>& source) {
            for (candidate& value : source) {
                value.score = nidx_t(rng() % 9) - 4;
                value.key = nidx_t(rng() % 5);
                value.evidence = nidx_t(rng());
            }
        };
        make(a); make(b); make(c);
        auto sa = keyed.id(), sb = keyed.id(), sc = keyed.id();
        for (candidate value : a) keyed.update(sa, move(value));
        for (candidate value : b) keyed.update(sb, move(value));
        for (candidate value : c) keyed.update(sc, move(value));
        check_state(sa, oracle<4>(a, true));
        check_state(keyed(sa, keyed.id()), values(sa));
        check_state(keyed(keyed.id(), sa), values(sa));
        CHECK(values(keyed(sa, sb)) == values(keyed(sb, sa)));
        vector<candidate> all = a;
        all.insert(all.end(), b.begin(), b.end());
        all.insert(all.end(), c.begin(), c.end());
        check_state(keyed(keyed(sa, sb), sc), oracle<4>(all, true));
        check_state(keyed(sa, keyed(sb, sc)), oracle<4>(all, true));
        CHECK(values(keyed(keyed(sa, sb), sc)) == values(keyed(sa, keyed(sb, sc))));
        CHECK(values(keyed(sa, sa)) == values(sa));
    }

    vector<ntopk<candidate, 4>> leaves;
    for (nidx_t i = 0; i < 23; ++i) leaves.push_back(op.single({i % 7, i % 5, i}));
    nseg<ntopk<candidate, 4>, ordinary> tree(leaves, op);
    for (nidx_t left = 0; left <= 23; ++left) for (nidx_t right = left; right <= 23; ++right) {
        auto expected = op.id();
        for (nidx_t i = left; i < right; ++i) expected = op(expected, leaves[i]);
        CHECK(values(tree.fold(left, right)) == values(expected));
    }
    tree.set(3, op.single({100, 3, 999}));
    CHECK(tree.fold(0, 23).front().score == 100);
    leaves[3] = op.single({-100, 3, 1000});
    tree.set(3, leaves[3]);
    vector<candidate> raw;
    for (const auto& leaf : leaves) raw.push_back(leaf.front());
    check_state(tree.fold(), oracle<4>(raw, false));
}
