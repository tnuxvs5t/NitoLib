#include "../src-v3/fhq.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct item {
    int handle;
    int value;
};

int main() {
    mt19937 rng(0x51A17);
    nfhq<int> q;
    q.reserve(30000);
    vector<int> roots{-1};
    vector<vector<item>> reference(1);

    auto verify = [&] {
        vector<unsigned char> seen(q.nodes());
        function<int(int, int)> dfs = [&](int root, int parent) -> int {
            if (root < 0) return 0;
            CHECK(!seen[root]);
            seen[root] = 1;
            CHECK(q[root].parent == parent);
            if (q[root].left >= 0) CHECK(q[root].priority >= q[q[root].left].priority);
            if (q[root].right >= 0) CHECK(q[root].priority >= q[q[root].right].priority);
            int count = 1 + dfs(q[root].left, root) + dfs(q[root].right, root);
            CHECK(q[root].size == count);
            return count;
        };
        int owned = 0;
        for (int t = 0; t < int(roots.size()); ++t) {
            CHECK(q.size(roots[t]) == int(reference[t].size()));
            CHECK(dfs(roots[t], -1) == int(reference[t].size()));
            for (int i = 0; i < int(reference[t].size()); ++i) {
                int handle = q.kth(roots[t], i);
                CHECK(handle == reference[t][i].handle);
                CHECK(q[handle].value == reference[t][i].value);
                CHECK(q.rank(handle) == i);
                CHECK(q.root_of(handle) == roots[t]);
            }
            owned += int(reference[t].size());
        }
        CHECK(owned == q.nodes());
        CHECK(count(seen.begin(), seen.end(), 1) == owned);
    };

    for (int round = 0; round < 30000; ++round) {
        int operation = int(rng() % 5);
        if (roots.size() > 25) operation = 2;

        if (operation == 0 || q.nodes() == 0) {
            int t = int(rng() % roots.size());
            int position = int(rng() % (reference[t].size() + 1));
            int value = int(rng());
            int handle = q.make(value);
            auto [left, right] = q.split(roots[t], position);
            roots[t] = q.merge(q.merge(left, handle), right);
            reference[t].insert(reference[t].begin() + position, {handle, value});
        } else if (operation == 1) {
            int t = int(rng() % roots.size());
            int position = int(rng() % (reference[t].size() + 1));
            auto [left, right] = q.split(roots[t], position);
            vector<item> tail(reference[t].begin() + position, reference[t].end());
            reference[t].erase(reference[t].begin() + position, reference[t].end());
            roots[t] = left;
            roots.push_back(right);
            reference.push_back(move(tail));
        } else if (operation == 2 && roots.size() > 1) {
            int a = int(rng() % roots.size()), b = int(rng() % (roots.size() - 1));
            if (b >= a) ++b;
            roots[a] = q.merge(roots[a], roots[b]);
            reference[a].insert(reference[a].end(), reference[b].begin(), reference[b].end());
            roots.erase(roots.begin() + b);
            reference.erase(reference.begin() + b);
        } else if (operation == 3 && roots.size() > 1) {
            int from = int(rng() % roots.size()), to = int(rng() % (roots.size() - 1));
            if (to >= from) ++to;
            int left = int(rng() % (reference[from].size() + 1));
            int right = left + int(rng() % (reference[from].size() - left + 1));
            int at = int(rng() % (reference[to].size() + 1));
            auto [ab, c] = q.split(roots[from], right);
            auto [a, b] = q.split(ab, left);
            roots[from] = q.merge(a, c);
            auto [x, y] = q.split(roots[to], at);
            roots[to] = q.merge(q.merge(x, b), y);
            vector<item> moved(reference[from].begin() + left, reference[from].begin() + right);
            reference[from].erase(reference[from].begin() + left, reference[from].begin() + right);
            reference[to].insert(reference[to].begin() + at, moved.begin(), moved.end());
        } else {
            int t = int(rng() % roots.size());
            auto values = q.sequence(roots[t]);
            for (int i = 0; i < values.len(); ++i) CHECK(values[i] == reference[t][i].value);
        }

        if (round % 97 == 0) verify();
    }
    verify();

    nfhq<int> ordered;
    for (int round = 0; round < 3000; ++round) {
        int n = int(rng() % 50), boundary = int(rng() % 31) - 15;
        vector<int> values(n);
        for (int& x : values) x = int(rng() % 31) - 15;
        sort(values.begin(), values.end());
        int root = ordered.build(nall(values));
        auto [left, right] = ordered.split_by(root, [&](int x) { return x < boundary; });
        int cut = int(lower_bound(values.begin(), values.end(), boundary) - values.begin());
        CHECK(ordered.size(left) == cut && ordered.size(right) == n - cut);
        root = ordered.merge(left, right);
        auto sequence = ordered.sequence(root);
        for (int i = 0; i < n; ++i) CHECK(sequence[i] == values[i]);
    }
}
