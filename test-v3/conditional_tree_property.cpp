#include "../src-v3/segment.hpp"
#include "../src-v3/fhq.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct stats {
    long long sum = 0, maximum = LLONG_MIN, second = LLONG_MIN;
    nidx_t count = 0, length = 0;
    static stats atom(long long x) { return {x, x, LLONG_MIN, 1, 1}; }
};

stats combine(const stats& a, const stats& b) {
    if (!a.length) return b;
    if (!b.length) return a;
    stats s{a.sum + b.sum, max(a.maximum, b.maximum), LLONG_MIN, 0, a.length + b.length};
    if (a.maximum == s.maximum) s.count += a.count;
    if (b.maximum == s.maximum) s.count += b.count;
    s.second = max(a.maximum == s.maximum ? a.second : a.maximum,
                   b.maximum == s.maximum ? b.second : b.maximum);
    return s;
}

struct chmin_command {
    long long cap;
    bool try_apply(stats& s) const {
        if (!s.length || s.maximum <= cap) return true;
        if (s.second >= cap) return false;
        s.sum -= (s.maximum - cap) * s.count;
        s.maximum = cap;
        return true;
    }
};

struct modulo_command {
    long long divisor;
    bool try_apply(stats& s) const {
        if (!s.length || s.maximum < divisor) return true;
        if (s.length != 1) return false;
        s = stats::atom(s.sum % divisor);
        return true;
    }
};

struct segment_ops {
    stats identity() const { return {}; }
    stats make(long long x) const { return stats::atom(x); }
    stats join(const stats& a, const stats& b) const { return combine(a, b); }
    void push(auto& q, nidx_t node, nidx_t, nidx_t) const {
        chmin_command pending{q[node].maximum};
        CHECK(pending.try_apply(q[node * 2]));
        CHECK(pending.try_apply(q[node * 2 + 1]));
    }
    template <class Q, class C>
    bool try_apply(Q& q, nidx_t node, nidx_t, nidx_t, const C& command) const {
        return command.try_apply(q[node]);
    }
};

struct item { long long x; stats aggregate; };

struct sequence_ops {
    void pull(auto& q, nidx_t root) const {
        auto& node = q[root];
        stats s = stats::atom(node.value.x);
        if (node.left >= 0) s = combine(q[node.left].value.aggregate, s);
        if (node.right >= 0) s = combine(s, q[node.right].value.aggregate);
        node.value.aggregate = s;
    }
    void push(auto& q, nidx_t root) const {
        chmin_command command{q[root].value.aggregate.maximum};
        for (nidx_t child : {q[root].left, q[root].right})
            if (child >= 0) CHECK(try_apply(q, child, command));
    }
    template <class Q, class C>
    void apply_one(Q& q, nidx_t root, const C& command) const {
        stats own = stats::atom(q[root].value.x);
        CHECK(command.try_apply(own));
        q[root].value.x = own.sum;
    }
    template <class Q, class C>
    bool try_apply(Q& q, nidx_t root, const C& command) const {
        if (!command.try_apply(q[root].value.aggregate)) return false;
        apply_one(q, root, command);
        return true;
    }
};

void check_stats(stats s, const vector<long long>& a, nidx_t left, nidx_t right) {
    vector<long long> ordered(a.begin() + left, a.begin() + right);
    sort(ordered.begin(), ordered.end(), greater<>{});
    CHECK(s.length == right - left);
    CHECK(s.sum == accumulate(ordered.begin(), ordered.end(), 0LL));
    if (ordered.empty()) { CHECK(s.count == 0); return; }
    CHECK(s.maximum == ordered.front());
    nidx_t count = 0;
    for (long long x : ordered) if (x == ordered.front()) ++count;
    CHECK(s.count == count);
    CHECK(s.second == (count == nidx_t(ordered.size()) ? LLONG_MIN : ordered[count]));
}

int main() {
    // Strict second-maximum boundary must reject without partially changing the node.
    stats boundary = combine(stats::atom(8), stats::atom(3));
    CHECK(!chmin_command{3}.try_apply(boundary));
    CHECK(boundary.sum == 11 && boundary.maximum == 8 && boundary.second == 3);
    CHECK(!modulo_command{5}.try_apply(boundary));
    CHECK(boundary.sum == 11);
    CHECK(chmin_command{4}.try_apply(boundary) && boundary.sum == 7);

    mt19937 rng(572991);
    for (nidx_t round = 0; round < 500; ++round) {
        nidx_t n = nidx_t(rng() % 101);
        vector<long long> values(n);
        for (auto& x : values) x = rng() % 10000;
        nlazyseg segment(nall(values), segment_ops{});
        nfhq<item, sequence_ops> q;
        nidx_t root = -1;
        for (auto x : values) root = q.merge(root, q.make(item{x, stats::atom(x)}));
        CHECK(segment.empty() == values.empty());
        for (nidx_t step = 0; step < 500; ++step) {
            nidx_t left = nidx_t(rng() % (n + 1)), right = nidx_t(rng() % (n + 1));
            if (left > right) swap(left, right);
            nidx_t op = nidx_t(rng() % 6);
            if (op < 2) {
                auto update = [&](const auto& command) {
                    segment.apply(left, right, command);
                    root = q.edit(root, left, right, [&](auto& tree, nidx_t middle) {
                        tree.apply(middle, command);
                        return middle;
                    });
                };
                if (op == 0) {
                    long long cap = rng() % 10000;
                    update(chmin_command{cap});
                    for (nidx_t i = left; i < right; ++i) values[i] = min(values[i], cap);
                } else {
                    long long divisor = 1 + rng() % 2000;
                    update(modulo_command{divisor});
                    for (nidx_t i = left; i < right; ++i) values[i] %= divisor;
                }
            } else if (op == 2 && n) {
                nidx_t position = nidx_t(rng() % n);
                values[position] = rng() % 10000;
                segment.set(position, values[position]);
                nidx_t handle = q.kth(root, position);
                q[handle].value.x = values[position];
                q.rebuild(handle);
            } else if (op == 3) {
                auto [ab, c] = q.split(root, right);
                auto [a, b] = q.split(ab, left);
                root = q.merge(b, q.merge(a, c));
                rotate(values.begin(), values.begin() + left, values.begin() + right);
                segment = nlazyseg(nall(values), segment_ops{});
            } else if (op == 4) {
                check_stats(segment.fold(left, right), values, left, right);
                root = q.edit(root, left, right, [&](auto& tree, nidx_t middle) {
                    check_stats(middle < 0 ? stats{} : tree[middle].value.aggregate,
                                values, left, right);
                    return middle;
                });
            } else {
                long long threshold = rng() % 10000;
                nidx_t expected = left;
                while (expected < right && values[expected] <= threshold) ++expected;
                nidx_t found = right;
                // A read-only prune is safe even on partial coverage. No fake tag.
                segment.walk(left, right, [&](auto& tree, nidx_t node, nidx_t lo,
                                               nidx_t hi, bool) {
                    if (found < right || tree[node].maximum <= threshold) return true;
                    if (hi - lo != 1) return false;
                    found = lo;
                    return true;
                });
                CHECK(found == expected);
                nidx_t offset = 0, seq_found = n;
                q.walk(root, [&](auto& tree, nidx_t node) {
                    if (seq_found < n) return true;
                    if (tree[node].value.aggregate.maximum > threshold) return false;
                    offset += tree.size(node);
                    return true;
                }, [&](auto& tree, nidx_t node) {
                    if (seq_found == n && tree[node].value.x > threshold) seq_found = offset;
                    ++offset;
                });
                auto first = find_if(values.begin(), values.end(),
                                     [&](long long x) { return x > threshold; });
                CHECK(seq_found == first - values.begin());
            }
            long long expected = accumulate(values.begin(), values.end(), 0LL);
            CHECK(segment.fold().sum == expected);
            CHECK((root < 0 ? 0 : q[root].value.aggregate.sum) == expected);
            if (step % 31 == 0) {
                check_stats(segment.fold(), values, 0, n);
                for (nidx_t i = 0; i < n; ++i) {
                    CHECK(segment.get(i).sum == values[i]);
                    CHECK(q[q.kth(root, i)].value.x == values[i]);
                }
            }
        }
    }

    // No lazy storage and no mandatory push/try_apply for ordinary point/fold use.
    struct plain_ops {
        unique_ptr<nidx_t> calls = make_unique<nidx_t>();
        long long identity() const { return 0; }
        long long make(long long x) const { return x; }
        long long join(long long a, long long b) { ++*calls; return a + b; }
    };
    vector<long long> tiny{1, 2, 3};
    nlazyseg plain(nall(tiny), plain_ops{});
    plain.set(1, 7);
    CHECK(plain.fold(0, 2) == 8 && *plain.ops.calls > 0);
    auto moved = move(plain);
    CHECK(moved.fold() == 11);

    nfhq<nidx_t> pool;
    vector<nidx_t> data{1, 2, 3};
    nidx_t root = pool.build(nall(data));
    vector<nidx_t> seen;
    pool.walk(root, [&](auto& tree, nidx_t) {
        for (nidx_t i = 0; i < 1000; ++i) tree.make(i);
        return false;
    }, [&](auto& tree, nidx_t node) { seen.push_back(tree[node].value); });
    CHECK(seen == data);
    root = pool.edit(root, 1, 2, [](auto& tree, nidx_t) { return tree.make(9); });
    CHECK(pool[pool.kth(root, 1)].value == 9);
    root = pool.edit(root, 0, 3, [](auto&, nidx_t) { return nidx_t(-1); });
    CHECK(root == -1);
    root = pool.edit(root, 0, 0, [](auto& tree, nidx_t middle) {
        CHECK(middle == -1);
        return tree.make(7);
    });
    CHECK(pool[root].value == 7 && pool.size(root) == 1);
}
