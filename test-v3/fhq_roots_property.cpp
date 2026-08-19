#include "../src-v3/fhq.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct item {
    nidx_t handle;
    nidx_t value;
};

int main() {
    mt19937 rng(0x51A17);
    nfhq<nidx_t> q;
    q.reserve(30000);
    vector<nidx_t> roots{-1};
    vector<vector<item>> reference(1);

    auto verify = [&] {
        vector<unsigned char> seen(q.nodes());
        function<nidx_t(nidx_t, nidx_t)> dfs = [&](nidx_t root, nidx_t parent) -> nidx_t {
            if (root < 0) return 0;
            CHECK(!seen[root]);
            seen[root] = 1;
            CHECK(q[root].parent == parent);
            if (q[root].left >= 0) CHECK(q[root].priority >= q[q[root].left].priority);
            if (q[root].right >= 0) CHECK(q[root].priority >= q[q[root].right].priority);
            nidx_t count = 1 + dfs(q[root].left, root) + dfs(q[root].right, root);
            CHECK(q[root].size == count);
            return count;
        };
        nidx_t owned = 0;
        for (nidx_t t = 0; t < nidx_t(roots.size()); ++t) {
            CHECK(q.size(roots[t]) == nidx_t(reference[t].size()));
            CHECK(dfs(roots[t], -1) == nidx_t(reference[t].size()));
            for (nidx_t i = 0; i < nidx_t(reference[t].size()); ++i) {
                nidx_t handle = q.kth(roots[t], i);
                CHECK(handle == reference[t][i].handle);
                CHECK(q[handle].value == reference[t][i].value);
                CHECK(q.rank(handle) == i);
                CHECK(q.root_of(handle) == roots[t]);
            }
            owned += nidx_t(reference[t].size());
        }
        CHECK(owned == q.nodes());
        CHECK(count(seen.begin(), seen.end(), 1) == owned);
    };

    for (nidx_t round = 0; round < 30000; ++round) {
        nidx_t operation = nidx_t(rng() % 5);
        if (roots.size() > 25) operation = 2;

        if (operation == 0 || q.nodes() == 0) {
            nidx_t t = nidx_t(rng() % roots.size());
            nidx_t position = nidx_t(rng() % (reference[t].size() + 1));
            nidx_t value = nidx_t(rng());
            nidx_t handle = q.make(value);
            auto [left, right] = q.split(roots[t], position);
            roots[t] = q.merge(q.merge(left, handle), right);
            reference[t].insert(reference[t].begin() + position, {handle, value});
        } else if (operation == 1) {
            nidx_t t = nidx_t(rng() % roots.size());
            nidx_t position = nidx_t(rng() % (reference[t].size() + 1));
            auto [left, right] = q.split(roots[t], position);
            vector<item> tail(reference[t].begin() + position, reference[t].end());
            reference[t].erase(reference[t].begin() + position, reference[t].end());
            roots[t] = left;
            roots.push_back(right);
            reference.push_back(move(tail));
        } else if (operation == 2 && roots.size() > 1) {
            nidx_t a = nidx_t(rng() % roots.size()), b = nidx_t(rng() % (roots.size() - 1));
            if (b >= a) ++b;
            roots[a] = q.merge(roots[a], roots[b]);
            reference[a].insert(reference[a].end(), reference[b].begin(), reference[b].end());
            roots.erase(roots.begin() + b);
            reference.erase(reference.begin() + b);
        } else if (operation == 3 && roots.size() > 1) {
            nidx_t from = nidx_t(rng() % roots.size()), to = nidx_t(rng() % (roots.size() - 1));
            if (to >= from) ++to;
            nidx_t left = nidx_t(rng() % (reference[from].size() + 1));
            nidx_t right = left + nidx_t(rng() % (reference[from].size() - left + 1));
            nidx_t at = nidx_t(rng() % (reference[to].size() + 1));
            auto [ab, c] = q.split(roots[from], right);
            auto [a, b] = q.split(ab, left);
            roots[from] = q.merge(a, c);
            auto [x, y] = q.split(roots[to], at);
            roots[to] = q.merge(q.merge(x, b), y);
            vector<item> moved(reference[from].begin() + left, reference[from].begin() + right);
            reference[from].erase(reference[from].begin() + left, reference[from].begin() + right);
            reference[to].insert(reference[to].begin() + at, moved.begin(), moved.end());
        } else {
            nidx_t t = nidx_t(rng() % roots.size());
            auto values = q.sequence(roots[t]);
            for (nidx_t i = 0; i < values.len(); ++i) CHECK(values[i] == reference[t][i].value);
        }

        if (round % 97 == 0) verify();
    }
    verify();

    nfhq<nidx_t> ordered;
    for (nidx_t round = 0; round < 3000; ++round) {
        nidx_t n = nidx_t(rng() % 50), boundary = nidx_t(rng() % 31) - 15;
        vector<nidx_t> values(n);
        for (nidx_t& x : values) x = nidx_t(rng() % 31) - 15;
        sort(values.begin(), values.end());
        nidx_t root = ordered.build(nall(values));
        auto [left, right] = ordered.split_by(root, [&](nidx_t x) { return x < boundary; });
        nidx_t cut = nidx_t(lower_bound(values.begin(), values.end(), boundary) - values.begin());
        CHECK(ordered.size(left) == cut && ordered.size(right) == n - cut);
        root = ordered.merge(left, right);
        auto sequence = ordered.sequence(root);
        for (nidx_t i = 0; i < n; ++i) CHECK(sequence[i] == values[i]);
    }
}
