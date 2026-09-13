#include "../src-v3/list.hpp"
#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

// A small unrolled list assembled from ordinary payloads, roots and views.
// sum includes lazy; data does not. No list-specific augmentation protocol.
struct block {
    vector<long long> data;
    long long sum = 0, lazy = 0;
    explicit block(vector<long long> values) : data(move(values)) { pull(); }
    void pull() { sum = accumulate(data.begin(), data.end(), 0LL); }
    void add(long long delta) { lazy += delta; sum += delta * nlen(data); }
    void push() {
        for (auto& x : nall(data)) x += lazy;
        lazy = 0;
    }
};

struct blocked_lists {
    static constexpr nidx_t width = 16;
    nlist<block> q;
    array<nlist<block>::root, 2> roots;

    // Return the node starting at the positional boundary, splitting a block
    // only if needed. Handles survive insert even when the node pool relocates.
    nidx_t split(nidx_t which, nidx_t position) {
        auto& root = roots[which];
        for (auto h = root.first; h >= 0; h = q.next(h)) {
            if (!position) return h;
            nidx_t size = nlen(q[h].data);
            if (position < size) {
                q[h].push();
                auto tail = ncollect(nsub(nall(q[h].data), position, size));
                q[h].data.resize(size_t(position));
                q[h].pull();
                return q.insert(root, q.next(h), move(tail));
            }
            position -= size;
        }
        CHECK(position == 0);
        return -1;
    }

    nlist<block>::root take(nidx_t which, nidx_t left, nidx_t right) {
        auto end = split(which, right), first = split(which, left);
        return q.cut(roots[which], first, end);
    }

    // Test-scale normalization: adjacent blocks totaling <= width are merged.
    // Production code can normalize only near edited boundaries.
    void compact(nidx_t which) {
        auto& root = roots[which];
        for (auto h = root.first; h >= 0;) {
            auto after = q.next(h);
            if (after >= 0 && nlen(q[h].data) + nlen(q[after].data) <= width) {
                q[h].push();
                q[after].push();
                for (auto x : nall(q[after].data)) q[h].data.push_back(x);
                q[h].pull();
                q.erase(root, after);
            } else h = after;
        }
    }
};

int main() {
    blocked_lists lists;
    auto& q = lists.q;
    array<vector<long long>, 2> oracle;
    // Fixed: both boundaries inside one lazy block, then move to another root.
    q.insert(lists.roots[0], -1, vector<long long>{1, 2, 3, 4, 5});
    q[lists.roots[0].first].add(10);
    auto middle = lists.take(0, 1, 4);
    q.splice(lists.roots[1], -1, middle);
    oracle[0] = {11, 15};
    oracle[1] = {12, 13, 14};

    auto verify = [&] {
        for (nidx_t which = 0; which < 2; ++which) {
            vector<long long> actual;
            nidx_t before = -1, blocks = 0;
            for (auto h = lists.roots[which].first; h >= 0; h = q.next(h)) {
                CHECK(++blocks <= nidx_t(oracle[which].size()));
                CHECK(q.prev(h) == before && !q[h].data.empty());
                CHECK(nlen(q[h].data) <= blocked_lists::width);
                long long sum = 0;
                for (auto x : nall(q[h].data)) {
                    actual.push_back(x + q[h].lazy);
                    sum += x + q[h].lazy;
                }
                CHECK(sum == q[h].sum);
                before = h;
            }
            CHECK(before == lists.roots[which].last && actual == oracle[which]);
        }
    };
    verify();
    mt19937 rng(719031);
    for (nidx_t step = 0; step < 12000; ++step) {
        nidx_t a = nidx_t(rng() % 2), b = 1 - a, n = nlen(oracle[a]);
        nidx_t left = nidx_t(rng() % (n + 1)), right = nidx_t(rng() % (n + 1));
        if (left > right) swap(left, right);
        nidx_t op = nidx_t(rng() % 6);
        if (!n || op == 0) {
            auto h = lists.split(a, left);
            vector<long long> values(1 + rng() % blocked_lists::width);
            for (auto& x : values) x = static_cast<long long>(rng() % 201) - 100;
            oracle[a].insert(oracle[a].begin() + left, values.begin(), values.end());
            q.insert(lists.roots[a], h, move(values));
        } else if (op == 1) {
            auto part = lists.take(a, left, right);
            q.clear(part);
            oracle[a].erase(oracle[a].begin() + left, oracle[a].begin() + right);
        } else if (op == 2 || op == 3) {
            auto end = lists.split(a, right), first = lists.split(a, left);
            long long delta = static_cast<long long>(rng() % 21) - 10, sum = 0;
            for (auto h = first; h != end; h = q.next(h)) {
                if (op == 2) q[h].add(delta);
                else sum += q[h].sum;
            }
            if (op == 2)
                for (nidx_t i = left; i < right; ++i) oracle[a][i] += delta;
            else CHECK(sum == accumulate(oracle[a].begin() + left, oracle[a].begin() + right, 0LL));
        } else if (op == 4) {
            auto part = lists.take(a, left, right);
            nidx_t position = nidx_t(rng() % (oracle[b].size() + 1));
            q.splice(lists.roots[b], lists.split(b, position), part);
            CHECK(part.empty());
            oracle[b].insert(oracle[b].begin() + position, oracle[a].begin() + left,
                             oracle[a].begin() + right);
            oracle[a].erase(oracle[a].begin() + left, oracle[a].begin() + right);
        } else {
            auto part = lists.take(a, left, right);
            for (auto h = part.first; h >= 0; h = q.next(h))
                ranges::reverse(nall(q[h].data));
            q.reverse(part);
            q.splice(lists.roots[a], lists.split(a, left), part);
            reverse(oracle[a].begin() + left, oracle[a].begin() + right);
        }
        lists.compact(a);
        lists.compact(b);
        verify();
    }
}
