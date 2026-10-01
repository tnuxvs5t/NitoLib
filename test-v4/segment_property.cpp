#include "../src-v4/segment.hpp"
#include "../src-v4/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

int main() {
    vector<nidx_t> values{3, 1, 4, 1, 5, 9};
    nseg<nidx_t> sum(nall(values));
    CHECK(sum.len() == 6 && sum.fold() == 23 && sum.fold(1, 4) == 6);
    sum.set(2, 10);
    CHECK(sum.get(2) == 10 && sum.fold() == 29);

    nseg<nidx_t, nmin<nidx_t>> minimum(nall(values));
    nseg<nidx_t, nmax<nidx_t>> maximum(nall(values));
    CHECK(minimum.fold() == 1 && maximum.fold() == 9);

    vector<string> letters{"a", "b", "c", "d", "e"};
    nseg ordered(nall(letters), concat{});
    CHECK(ordered.fold(1, 4) == "bcd");
    CHECK(ordered.max_right(1, [](const string& x) { return string("bcd").starts_with(x); }) == 4);
    CHECK(ordered.min_left(4, [](const string& x) { return string("bcd").ends_with(x); }) == 1);

    nseg<nidx_t> other(nall(values));
    other.set(0, 1);
    sum.pointwise(other);
    CHECK(sum.get(0) == 4 && sum.get(2) == 14);

    vector<nidx_t> trace, cover;
    nsegment_trace(sum.base, 4, [&](nidx_t node) { trace.push_back(node); });
    nsegment_cover(sum.base, 1, 5, [&](nidx_t node, nidx_t left, nidx_t right) {
        CHECK(left < right);
        cover.push_back(node);
    });
    CHECK(!trace.empty() && !cover.empty());

    mt19937 rng(0x5E6);
    for (int round = 0; round < 5000; ++round) {
        nidx_t n = nidx_t(rng() % 65);
        vector<nidx_t> a(n);
        for (nidx_t& x : a) x = nidx_t(rng() % 101) - 50;
        nseg<nidx_t> tree(nall(a));
        for (int query = 0; query < 30; ++query) {
            nidx_t left = n ? nidx_t(rng() % (n + 1)) : 0;
            nidx_t right = left + (n - left ? nidx_t(rng() % (n - left + 1)) : 0);
            nidx_t expected = accumulate(a.begin() + left, a.begin() + right, nidx_t(0));
            CHECK(tree.fold(left, right) == expected);
        }
        if (n) {
            nidx_t position = nidx_t(rng() % n), value = nidx_t(rng() % 101) - 50;
            a[position] = value;
            tree.set(position, value);
            CHECK(tree.fold() == accumulate(a.begin(), a.end(), nidx_t(0)));
        }
    }

    nlazy_addsum<long long> lazy(nidx_t(6));
    lazy.apply(0, 6, 3LL);
    lazy.set(2, -4LL);
    CHECK(lazy.fold() == 11 && lazy.get(2) == -4);
    CHECK(lazy.fold(1, 5) == 5);
    nlazy_addsum<long long> monotone(nidx_t(6));
    monotone.apply(0, 6, 3LL);
    CHECK(monotone.max_right(0, [](long long sum) { return sum <= 5; }) == 1);
    CHECK(monotone.min_left(6, [](long long sum) { return sum <= 5; }) == 5);

    mt19937_64 lazy_rng(0xA11CE);
    for (int round = 0; round < 3000; ++round) {
        nidx_t n = nidx_t(lazy_rng() % 40);
        vector<long long> expected(n);
        nlazy_addsum<long long> tree(nall(expected));
        for (int step = 0; step < 80; ++step) {
            nidx_t left = n ? nidx_t(lazy_rng() % (n + 1)) : 0;
            nidx_t right = left + (n - left ? nidx_t(lazy_rng() % (n - left + 1)) : 0);
            if (lazy_rng() & 1) {
                long long delta = static_cast<long long>(lazy_rng() % 21) - 10;
                tree.apply(left, right, delta);
                for (nidx_t i = left; i < right; ++i) expected[i] += delta;
            } else if (n) {
                long long value = static_cast<long long>(lazy_rng() % 101) - 50;
                nidx_t position = nidx_t(lazy_rng() % n);
                tree.set(position, value);
                expected[position] = value;
            }
            if (n) {
                nidx_t ql = nidx_t(lazy_rng() % (n + 1));
                nidx_t qr = ql + (n - ql ? nidx_t(lazy_rng() % (n - ql + 1)) : 0);
                CHECK(tree.fold(ql, qr) == accumulate(expected.begin() + ql,
                                                       expected.begin() + qr, 0LL));
                for (nidx_t i = 0; i < n; ++i) CHECK(tree.get(i) == expected[i]);
            } else {
                CHECK(tree.empty() && tree.fold() == 0);
            }
        }
    }

    cout << "v4 segment: topology, folds, boundary search and pointwise merge passed\n";
}
