#include "../src-v3/list.hpp"
#include "../src-v3/graph.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct immobile {
    static inline nidx_t alive = 0;
    nidx_t value;
    explicit immobile(nidx_t x) : value(x) { ++alive; }
    immobile(const immobile&) = delete;
    immobile(immobile&&) = delete;
    ~immobile() { --alive; }
};

struct throwing_copy {
    static inline nidx_t alive = 0, remaining = 100;
    throwing_copy() { ++alive; }
    throwing_copy(const throwing_copy&) {
        if (!remaining--) throw runtime_error("copy");
        ++alive;
    }
    ~throwing_copy() { --alive; }
};

int main() {
    static_assert(bidirectional_iterator<nlist<nidx_t>::iterator>);
    static_assert(bidirectional_iterator<nlist<nidx_t>::const_iterator>);
    static_assert(!random_access_iterator<nlist<nidx_t>::iterator>);
    static_assert(same_as<decltype(*declval<const nlist<nidx_t>&>().begin()), const nidx_t&>);
    static_assert(!is_copy_constructible_v<nlist<immobile>>);
    {
        nlist<immobile> a, b;
        auto saved = a.emplace(a.end(), 7);
        immobile* address = addressof(*saved);
        a.emplace_front(2);
        b.splice(b.end(), a, saved);
        CHECK(a.len() == 1 && b.len() == 1 && addressof(b.front()) == address);
        b.emplace_back(9);
        CHECK(next(saved)->value == 9);
        nlist<immobile> moved(move(b));
        CHECK(b.empty() && addressof(moved.front()) == address);
        CHECK(next(next(saved)) == moved.end());
        a = move(moved);
        CHECK(moved.empty() && immobile::alive == 2 && addressof(a.front()) == address);
        a.reverse();
        CHECK(a.back().value == 7 && addressof(a.back()) == address);
        a.erase(saved);
        CHECK(immobile::alive == 1);
        a.clear();
        CHECK(immobile::alive == 0);
    }
    {
        nlist<throwing_copy> source, target;
        source.emplace_back(); source.emplace_back(); source.emplace_back();
        target.emplace_back();
        auto* old = addressof(target.front());
        throwing_copy::remaining = 1;
        bool threw = false;
        try { target = source; } catch (const runtime_error&) { threw = true; }
        CHECK(threw && throwing_copy::alive == 4 && addressof(target.front()) == old);
        throwing_copy::remaining = 0;
        try { target.emplace(target.end(), source.front()); } catch (const runtime_error&) {}
        CHECK(target.len() == 1 && throwing_copy::alive == 4);
    }
    CHECK(throwing_copy::alive == 0);

    array<nlist<nidx_t>, 3> lists;
    array<vector<nidx_t>, 3> oracle;
    unordered_map<nidx_t, nidx_t*> addresses;
    mt19937 rng(0x1157);
    nidx_t serial = 0;
    auto at = [&](nidx_t which, nidx_t position) {
        return next(lists[which].begin(), position);
    };
    for (nidx_t step = 0; step < 25000; ++step) {
        nidx_t a = nidx_t(rng() % 3), b = nidx_t(rng() % 3);
        nidx_t n = lists[a].len(), m = lists[b].len();
        nidx_t p = nidx_t(rng() % (n + 1)), op = nidx_t(rng() % 8);
        if (op == 0 || (!n && op == 1)) {
            auto it = lists[a].insert(at(a, p), serial);
            addresses[serial] = addressof(*it);
            oracle[a].insert(oracle[a].begin() + p, serial++);
        } else if (op == 1 && n) {
            p %= n;
            addresses.erase(oracle[a][p]);
            auto after = lists[a].erase(at(a, p));
            oracle[a].erase(oracle[a].begin() + p);
            CHECK(after == at(a, p));
        } else if (op == 2) {
            nidx_t l = nidx_t(rng() % (m + 1)), r = nidx_t(rng() % (m + 1));
            if (l > r) swap(l, r);
            if (a == b && l < p && p < r) continue;
            lists[a].splice(at(a, p), lists[b], at(b, l), at(b, r));
            if (!(a == b && (p == l || p == r))) {
                vector<nidx_t> part(oracle[b].begin() + l, oracle[b].begin() + r);
                oracle[b].erase(oracle[b].begin() + l, oracle[b].begin() + r);
                if (a == b && p > r) p -= r - l;
                oracle[a].insert(oracle[a].begin() + p, part.begin(), part.end());
            }
        } else if (op == 3) {
            lists[a].splice(at(a, p), lists[b]);
            if (a != b) {
                oracle[a].insert(oracle[a].begin() + p, oracle[b].begin(), oracle[b].end());
                oracle[b].clear();
            }
        } else if (op == 4 && m) {
            nidx_t q = nidx_t(rng() % m), value = oracle[b][q];
            lists[a].splice(at(a, p), lists[b], at(b, q));
            if (!(a == b && (p == q || p == q + 1))) {
                oracle[b].erase(oracle[b].begin() + q);
                if (a == b && p > q) --p;
                oracle[a].insert(oracle[a].begin() + p, value);
            }
        } else if (op == 5) {
            lists[a].reverse();
            reverse(oracle[a].begin(), oracle[a].end());
        } else if (op == 6) {
            nlist<nidx_t> copy(lists[a]);
            CHECK(vector<nidx_t>(copy.begin(), copy.end()) == oracle[a]);
            if (!copy.empty()) CHECK(addressof(copy.front()) != addressof(lists[a].front()));
            copy = copy;
            CHECK(vector<nidx_t>(copy.begin(), copy.end()) == oracle[a]);
            auto& alias = lists[a];
            lists[a] = move(alias);
        } else if (op == 7) {
            nidx_t r = p + nidx_t(rng() % (n - p + 1));
            for (nidx_t i = p; i < r; ++i) addresses.erase(oracle[a][i]);
            auto after = lists[a].erase(at(a, p), at(a, r));
            oracle[a].erase(oracle[a].begin() + p, oracle[a].begin() + r);
            CHECK(after == at(a, p));
        }
        for (nidx_t which = 0; which < 3; ++which) {
            const auto& list = lists[which];
            CHECK(list.len() == nidx_t(oracle[which].size()));
            auto it = list.begin();
            for (nidx_t value : oracle[which]) {
                CHECK(it != list.end() && *it == value && addressof(*it) == addresses.at(value));
                ++it;
            }
            CHECK(it == list.end());
            for (auto expected = oracle[which].rbegin(); expected != oracle[which].rend(); ++expected)
                CHECK(*--it == *expected);
            CHECK(it == list.begin());
            CHECK(lists[which].begin() == list.begin());
        }
    }

    array<nlist<nidx_t>, 4> adjacency;
    adjacency[0].push_back(2); adjacency[2].push_back(3);
    auto graph = ngraph{nrange(4), [&](nidx_t u) -> auto& { return adjacency[u]; }};
    CHECK((nbfs(graph, 0) == vector<nidx_t>{0, -1, 1, 2}));
}
