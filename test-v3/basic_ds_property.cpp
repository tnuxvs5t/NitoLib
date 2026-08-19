#include "../src-v3/ds.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

struct minimum {
    nidx_t operator()(nidx_t a, nidx_t b) const { return min(a, b); }
};

int main() {
    mt19937 rng(0xD5);
    for (nidx_t round = 0; round < 5000; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 100);
        vector<long long> values(n);
        nfenwick<long long> tree(nall(values));
        for (nidx_t step = 0; step < 300; ++step) {
            nidx_t operation = nidx_t(rng() % 4), position = nidx_t(rng() % n);
            if (operation == 0) {
                long long delta = rng() % 30;
                values[position] += delta;
                tree.add(position, delta);
            } else if (operation == 1) {
                long long value = rng() % 100;
                values[position] = value;
                tree.set(position, value);
            } else {
                nidx_t left = nidx_t(rng() % (n + 1));
                nidx_t right = left + nidx_t(rng() % (n - left + 1));
                CHECK(tree.fold(left, right) ==
                      accumulate(values.begin() + left, values.begin() + right, 0LL));
            }
            long long total = accumulate(values.begin(), values.end(), 0LL);
            long long target = rng() % (total + 2);
            nidx_t expected = n;
            long long prefix = 0;
            for (nidx_t i = 0; i < n; ++i) {
                prefix += values[i];
                if (prefix >= target) { expected = i; break; }
            }
            CHECK(tree.lower(target) == expected);
        }
    }

    for (nidx_t round = 0; round < 3000; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 30);
        nrollback_dsu rollback(n);
        vector<nidx_t> label(n);
        iota(label.begin(), label.end(), 0);
        vector<vector<nidx_t>> snapshots{label};
        for (nidx_t step = 0; step < 200; ++step) {
            if (rng() % 4 || rollback.time() == 0) {
                nidx_t a = nidx_t(rng() % n), b = nidx_t(rng() % n);
                bool expected = label[a] != label[b];
                nidx_t old = label[b];
                if (expected)
                    for (nidx_t& value : label) if (value == old) value = label[a];
                CHECK(rollback.merge(a, b) == expected);
                if (expected) snapshots.push_back(label);
            } else {
                nidx_t target = nidx_t(rng() % (rollback.time() + 1));
                rollback.rollback(target);
                label = snapshots[target];
                snapshots.resize(target + 1);
            }
            nidx_t probe = nidx_t(rng() % n);
            for (nidx_t vertex = 0; vertex < n; ++vertex)
                CHECK(rollback.same(probe, vertex) == (label[probe] == label[vertex]));
        }
        for (nidx_t a = 0; a < n; ++a)
            for (nidx_t b = 0; b < n; ++b)
                CHECK(rollback.same(a, b) == (label[a] == label[b]));

        ndsu compressed(n);
        iota(label.begin(), label.end(), 0);
        for (nidx_t step = 0; step < 200; ++step) {
            nidx_t a = nidx_t(rng() % n), b = nidx_t(rng() % n), old = label[b];
            if (label[a] != old)
                for (nidx_t& value : label) if (value == old) value = label[a];
            compressed.merge(a, b);
            for (nidx_t vertex = 0; vertex < n; ++vertex)
                CHECK(compressed.same(a, vertex) == (label[a] == label[vertex]));
        }
    }

    for (nidx_t round = 0; round < 5000; ++round) {
        nqueue_agg<string, concat> queue;
        deque<string> reference;
        for (nidx_t step = 0; step < 300; ++step) {
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
            CHECK(queue.fold() == expected && queue.len() == nidx_t(reference.size()));
        }
    }

    for (nidx_t round = 0; round < 5000; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 100);
        vector<nidx_t> values(n);
        for (nidx_t& value : values) value = nidx_t(rng());
        nsparse_table table(nall(values), minimum{});
        for (nidx_t query = 0; query < 200; ++query) {
            nidx_t left = nidx_t(rng() % n);
            nidx_t right = left + 1 + nidx_t(rng() % (n - left));
            CHECK(table.fold(left, right) == *min_element(values.begin() + left,
                                                          values.begin() + right));
        }
    }
}
