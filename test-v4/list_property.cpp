#include "../src-v4/list.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct payload {
    static inline nidx_t live = 0;
    unique_ptr<nidx_t> value;

    explicit payload(nidx_t x) : value(make_unique<nidx_t>(x)) {
        if (x < 0) throw runtime_error("construction");
        ++live;
    }
    payload(payload&& other) noexcept : value(move(other.value)) { ++live; }
    ~payload() { --live; }
};

int main() {
    {
        nlist<payload> list;
        nlist<payload>::root a, b;
        nidx_t first = list.insert(a, -1, 7);
        nidx_t second = list.insert(a, -1, 8);
        auto part = list.cut(a, first, second);
        CHECK(a.first == second && a.last == second && list.prev(second) < 0);
        list.splice(b, -1, part);
        CHECK(part.empty() && list[first].value && *list[first].value == 7);
        list.reserve(1000);
        CHECK(*list[first].value == 7);
        CHECK(list.erase(b, first) < 0 && b.empty() && payload::live == 1);
        bool threw = false;
        try { list.insert(a, -1, -1); } catch (const runtime_error&) { threw = true; }
        CHECK(threw && list.free == first && !list.pool[first].value);
        CHECK(list.insert(a, second, 9) == first);
        auto all = list.cut(a, a.first, -1);
        list.reverse(all);
        list.splice(a, -1, all);
        list.clear(a);
        CHECK(payload::live == 0 && list.pool.len() == 2);
    }
    CHECK(payload::live == 0);

    nlist<nidx_t> list;
    array<nlist<nidx_t>::root, 3> roots;
    array<vector<nidx_t>, 3> oracle;
    unordered_map<nidx_t, nidx_t> handle;
    mt19937 rng(0x1157);
    nidx_t serial = 0, peak = 0;

    auto at = [&](nidx_t chain, nidx_t position) {
        return position == nidx_t(oracle[chain].size())
                   ? nidx_t(-1) : handle.at(oracle[chain][position]);
    };

    for (nidx_t step = 0; step < 18000; ++step) {
        nidx_t a = nidx_t(rng() % 3), b = nidx_t(rng() % 3);
        nidx_t n = nidx_t(oracle[a].size());
        nidx_t m = nidx_t(oracle[b].size());
        nidx_t p = nidx_t(rng() % (n + 1));
        nidx_t op = nidx_t(rng() % 6);
        if (op == 0 || (!n && op == 1)) {
            nidx_t value = serial++;
            handle[value] = list.insert(roots[a], at(a, p), value);
            oracle[a].insert(oracle[a].begin() + p, value);
        } else if (op == 1) {
            p %= n;
            nidx_t value = oracle[a][p], after = at(a, p + 1);
            CHECK(list.erase(roots[a], handle.at(value)) == after);
            handle.erase(value);
            oracle[a].erase(oracle[a].begin() + p);
        } else if (op == 2 || op == 3) {
            nidx_t l = nidx_t(rng() % (m + 1));
            nidx_t r = nidx_t(rng() % (m + 1));
            if (l > r) swap(l, r);
            if (a == b && l <= p && p < r) continue;
            nidx_t position = at(a, p);
            auto part = list.cut(roots[b], at(b, l), at(b, r));
            vector<nidx_t> values(oracle[b].begin() + l, oracle[b].begin() + r);
            oracle[b].erase(oracle[b].begin() + l, oracle[b].begin() + r);
            if (op == 3) {
                list.reverse(part);
                ranges::reverse(values);
            }
            list.splice(roots[a], position, part);
            if (a == b && p >= r) p -= r - l;
            oracle[a].insert(oracle[a].begin() + p, values.begin(), values.end());
        } else if (op == 4) {
            list.reverse(roots[a]);
            ranges::reverse(oracle[a]);
        } else {
            list.clear(roots[a]);
            for (nidx_t value : oracle[a]) handle.erase(value);
            oracle[a].clear();
        }

        peak = max(peak, nidx_t(handle.size()));
        CHECK(list.pool.len() == peak);
        vector<unsigned char> seen(size_t(list.pool.len()));
        for (nidx_t chain = 0; chain < 3; ++chain) {
            nidx_t h = roots[chain].first, before = -1;
            for (nidx_t value : oracle[chain]) {
                CHECK(h >= 0 && h < list.pool.len() && !seen[h]);
                seen[h] = true;
                CHECK(list.pool[h].value && list[h] == value);
                CHECK(list.prev(h) == before);
                before = h;
                h = list.next(h);
            }
            CHECK(h < 0 && before == roots[chain].last);
        }
        for (nidx_t h = list.free; h >= 0; h = list.next(h)) {
            CHECK(h < list.pool.len() && !seen[h] && !list.pool[h].value);
            seen[h] = true;
        }
        CHECK(ranges::all_of(seen, identity{}));
    }
}
