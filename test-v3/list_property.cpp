#include "../src-v3/list.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct payload {
    static inline nidx_t alive = 0;
    unique_ptr<nidx_t> value;
    explicit payload(nidx_t x) : value(make_unique<nidx_t>(x)) {
        if (x < 0) throw runtime_error("construction");
        ++alive;
    }
    payload(payload&& other) noexcept : value(move(other.value)) { ++alive; }
    ~payload() { --alive; }
};

int main() {
    static_assert(same_as<decltype(declval<const nlist<nidx_t>&>()[0]), const nidx_t&>);
    {
        nlist<payload> q;
        nlist<payload>::root a, b;
        auto x = q.insert(a, -1, 7), y = q.insert(a, -1, 8);
        auto* owned = q[x].value.get();
        auto part = q.cut(a, x, y);
        CHECK(a.first == y && a.last == y && q.prev(y) == -1);
        q.splice(b, -1, part);
        CHECK(part.empty() && part.last == -1 && q[x].value.get() == owned);
        q.reserve(1000);
        CHECK(q[x].value.get() == owned && *q[x].value == 7);
        CHECK(q.erase(b, x) == -1 && b.empty() && b.last == -1 && payload::alive == 1);
        bool threw = false;
        try { q.insert(a, -1, -1); } catch (const runtime_error&) { threw = true; }
        CHECK(threw && q.free == x && !q.pool[x].value && payload::alive == 1);
        CHECK(q.insert(a, y, 9) == x && q.pool.len() == 2);
        auto all = q.cut(a, a.first, -1);
        CHECK(a.empty() && a.last == -1);
        q.reverse(all);
        CHECK(all.first == y && all.last == x);
        q.splice(a, -1, all);
        q.clear(a);
        CHECK(payload::alive == 0 && q.pool.len() == 2);
        for (nidx_t i = 0; i < 1000; ++i) {
            q.insert(a, -1, i);
            q.clear(a);
        }
        CHECK(q.pool.len() == 2 && payload::alive == 0);
    }
    CHECK(payload::alive == 0);

    nlist<nidx_t> q;
    array<nlist<nidx_t>::root, 3> chains;
    array<vector<nidx_t>, 3> oracle;
    unordered_map<nidx_t, nidx_t> handles;
    mt19937 rng(0x1157);
    nidx_t serial = 0, peak = 0;
    auto at = [&](nidx_t a, nidx_t p) {
        return p == nidx_t(oracle[a].size()) ? nidx_t(-1) : handles.at(oracle[a][p]);
    };
    for (nidx_t step = 0; step < 25000; ++step) {
        nidx_t a = nidx_t(rng() % 3), b = nidx_t(rng() % 3);
        nidx_t n = nidx_t(oracle[a].size()), m = nidx_t(oracle[b].size());
        nidx_t p = nidx_t(rng() % (n + 1)), op = nidx_t(rng() % 6);
        if (op == 0 || (!n && op == 1)) {
            handles[serial] = q.insert(chains[a], at(a, p), serial);
            oracle[a].insert(oracle[a].begin() + p, serial++);
        } else if (op == 1) {
            p %= n;
            nidx_t value = oracle[a][p], expected = at(a, p + 1);
            CHECK(q.erase(chains[a], handles.at(value)) == expected);
            handles.erase(value);
            oracle[a].erase(oracle[a].begin() + p);
        } else if (op == 2 || op == 3) {
            nidx_t l = nidx_t(rng() % (m + 1)), r = nidx_t(rng() % (m + 1));
            if (l > r) swap(l, r);
            if (a == b && l <= p && p < r) continue;
            nidx_t position = at(a, p);
            auto part = q.cut(chains[b], at(b, l), at(b, r));
            vector<nidx_t> values(oracle[b].begin() + l, oracle[b].begin() + r);
            oracle[b].erase(oracle[b].begin() + l, oracle[b].begin() + r);
            if (op == 3) {
                q.reverse(part);
                reverse(values.begin(), values.end());
            }
            q.splice(chains[a], position, part);
            CHECK(part.empty() && part.last == -1);
            if (a == b && p >= r) p -= r - l;
            oracle[a].insert(oracle[a].begin() + p, values.begin(), values.end());
        } else if (op == 4) {
            q.reverse(chains[a]);
            reverse(oracle[a].begin(), oracle[a].end());
        } else {
            q.clear(chains[a]);
            for (auto value : oracle[a]) handles.erase(value);
            oracle[a].clear();
        }
        peak = max(peak, nidx_t(handles.size()));
        CHECK(q.pool.len() == peak);
        vector<bool> seen(size_t(q.pool.len()));
        for (nidx_t which = 0; which < 3; ++which) {
            auto chain = chains[which];
            nidx_t h = chain.first, before = -1;
            for (auto value : oracle[which]) {
                CHECK(h >= 0 && h < q.pool.len() && !seen[h]);
                seen[h] = true;
                CHECK(q.pool[h].value && q[h] == value && handles.at(value) == h);
                CHECK(q.prev(h) == before);
                before = h;
                h = q.next(h);
            }
            CHECK(h == -1 && before == chain.last);
            h = chain.last;
            for (auto it = oracle[which].rbegin(); it != oracle[which].rend(); ++it) {
                CHECK(h == handles.at(*it));
                h = q.prev(h);
            }
            CHECK(h == -1 && chain.empty() == oracle[which].empty());
        }
        for (auto h = q.free; h >= 0; h = q.next(h)) {
            CHECK(h < q.pool.len() && !seen[h] && !q.pool[h].value);
            seen[h] = true;
        }
        CHECK(all_of(seen.begin(), seen.end(), [](bool x) { return x; }));
    }
}
