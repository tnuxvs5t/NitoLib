#include "../src-v3/ds.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

struct minimum {
    int operator()(int a, int b) const { return min(a, b); }
};

int main() {
    mt19937 rng(0xD5);
    for (int round = 0; round < 5000; ++round) {
        int n = 1 + int(rng() % 100);
        vector<long long> values(n);
        nfenwick<long long> tree(nall(values));
        for (int step = 0; step < 300; ++step) {
            int operation = int(rng() % 4), position = int(rng() % n);
            if (operation == 0) {
                long long delta = rng() % 30;
                values[position] += delta;
                tree.add(position, delta);
            } else if (operation == 1) {
                long long value = rng() % 100;
                values[position] = value;
                tree.set(position, value);
            } else {
                int left = int(rng() % (n + 1));
                int right = left + int(rng() % (n - left + 1));
                CHECK(tree.fold(left, right) ==
                      accumulate(values.begin() + left, values.begin() + right, 0LL));
            }
            long long total = accumulate(values.begin(), values.end(), 0LL);
            long long target = rng() % (total + 2);
            int expected = n;
            long long prefix = 0;
            for (int i = 0; i < n; ++i) {
                prefix += values[i];
                if (prefix >= target) { expected = i; break; }
            }
            CHECK(tree.lower(target) == expected);
        }
    }

    for (int round = 0; round < 3000; ++round) {
        int n = 1 + int(rng() % 30);
        nrollback_dsu rollback(n);
        vector<int> label(n);
        iota(label.begin(), label.end(), 0);
        vector<vector<int>> snapshots{label};
        for (int step = 0; step < 200; ++step) {
            if (rng() % 4 || rollback.time() == 0) {
                int a = int(rng() % n), b = int(rng() % n);
                bool expected = label[a] != label[b];
                int old = label[b];
                if (expected)
                    for (int& value : label) if (value == old) value = label[a];
                CHECK(rollback.merge(a, b) == expected);
                if (expected) snapshots.push_back(label);
            } else {
                int target = int(rng() % (rollback.time() + 1));
                rollback.rollback(target);
                label = snapshots[target];
                snapshots.resize(target + 1);
            }
            int probe = int(rng() % n);
            for (int vertex = 0; vertex < n; ++vertex)
                CHECK(rollback.same(probe, vertex) == (label[probe] == label[vertex]));
        }
        for (int a = 0; a < n; ++a)
            for (int b = 0; b < n; ++b)
                CHECK(rollback.same(a, b) == (label[a] == label[b]));

        ndsu compressed(n);
        iota(label.begin(), label.end(), 0);
        for (int step = 0; step < 200; ++step) {
            int a = int(rng() % n), b = int(rng() % n), old = label[b];
            if (label[a] != old)
                for (int& value : label) if (value == old) value = label[a];
            compressed.merge(a, b);
            for (int vertex = 0; vertex < n; ++vertex)
                CHECK(compressed.same(a, vertex) == (label[a] == label[vertex]));
        }
    }

    for (int round = 0; round < 5000; ++round) {
        nqueue_agg<string, concat> queue;
        deque<string> reference;
        for (int step = 0; step < 300; ++step) {
            if (reference.empty() || rng() & 1) {
                string value(1, char('a' + rng() % 5));
                queue.push(value);
                reference.push_back(value);
            } else {
                CHECK(queue.front() == reference.front());
                queue.pop();
                reference.pop_front();
            }
            string expected;
            for (const string& value : reference) expected += value;
            CHECK(queue.fold() == expected && queue.len() == int(reference.size()));
        }
    }

    for (int round = 0; round < 5000; ++round) {
        int n = 1 + int(rng() % 100);
        vector<int> values(n);
        for (int& value : values) value = int(rng());
        nsparse_table table(nall(values), minimum{});
        for (int query = 0; query < 200; ++query) {
            int left = int(rng() % n);
            int right = left + 1 + int(rng() % (n - left));
            CHECK(table.fold(left, right) == *min_element(values.begin() + left,
                                                          values.begin() + right));
        }
    }
}
