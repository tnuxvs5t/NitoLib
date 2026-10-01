#include "../src-v4/fhq.hpp"
#include "../src-v4/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct item {
    nidx_t handle;
    nidx_t value;
};

int main() {
    mt19937 rng(0x51A17);
    nfhq<nidx_t> tree;
    vector<nidx_t> roots{-1};
    vector<vector<item>> reference(1);

    auto verify = [&] {
        vector<unsigned char> seen(tree.nodes());
        auto check = [&](auto&& self, nidx_t root, nidx_t parent) -> nidx_t {
            if (root < 0) return 0;
            CHECK(!seen[root]);
            seen[root] = 1;
            CHECK(tree[root].parent == parent);
            if (tree[root].left >= 0)
                CHECK(tree[root].priority >= tree[tree[root].left].priority);
            if (tree[root].right >= 0)
                CHECK(tree[root].priority >= tree[tree[root].right].priority);
            nidx_t count = 1 + self(self, tree[root].left, root) +
                                 self(self, tree[root].right, root);
            CHECK(tree[root].size == count);
            return count;
        };

        nidx_t owned = 0;
        for (nidx_t t = 0; t < nidx_t(roots.size()); ++t) {
            CHECK(tree.size(roots[t]) == nidx_t(reference[t].size()));
            CHECK(check(check, roots[t], -1) == nidx_t(reference[t].size()));
            for (nidx_t i = 0; i < nidx_t(reference[t].size()); ++i) {
                nidx_t handle = tree.kth(roots[t], i);
                CHECK(handle == reference[t][i].handle);
                CHECK(tree[handle].value == reference[t][i].value);
                CHECK(tree.rank(handle) == i && tree.root_of(handle) == roots[t]);
            }
            owned += nidx_t(reference[t].size());
        }
        CHECK(owned == tree.nodes());
        CHECK(count(seen.begin(), seen.end(), 1) == owned);
    };

    for (int round = 0; round < 8000; ++round) {
        nidx_t action = nidx_t(rng() % 5);
        if (roots.size() > 20) action = 2;

        if (action == 0 || tree.nodes() == 0) {
            nidx_t t = nidx_t(rng() % roots.size());
            nidx_t at = nidx_t(rng() % (reference[t].size() + 1));
            nidx_t value = nidx_t(rng() % 1000);
            nidx_t handle = tree.make(value);
            auto [left, right] = tree.split(roots[t], at);
            roots[t] = tree.merge(tree.merge(left, handle), right);
            reference[t].insert(reference[t].begin() + at, {handle, value});
        } else if (action == 1) {
            nidx_t t = nidx_t(rng() % roots.size());
            nidx_t at = nidx_t(rng() % (reference[t].size() + 1));
            auto [left, right] = tree.split(roots[t], at);
            vector<item> tail(reference[t].begin() + at, reference[t].end());
            reference[t].erase(reference[t].begin() + at, reference[t].end());
            roots[t] = left;
            roots.push_back(right);
            reference.push_back(move(tail));
        } else if (action == 2 && roots.size() > 1) {
            nidx_t a = nidx_t(rng() % roots.size());
            nidx_t b = nidx_t(rng() % (roots.size() - 1));
            if (b >= a) ++b;
            roots[a] = tree.merge(roots[a], roots[b]);
            reference[a].insert(reference[a].end(), reference[b].begin(), reference[b].end());
            roots.erase(roots.begin() + b);
            reference.erase(reference.begin() + b);
        } else if (action == 3 && roots.size() > 1) {
            nidx_t from = nidx_t(rng() % roots.size());
            nidx_t to = nidx_t(rng() % (roots.size() - 1));
            if (to >= from) ++to;
            nidx_t left = nidx_t(rng() % (reference[from].size() + 1));
            nidx_t right = left + nidx_t(rng() % (reference[from].size() - left + 1));
            nidx_t at = nidx_t(rng() % (reference[to].size() + 1));

            auto [prefix, suffix] = tree.split(roots[from], right);
            auto [head, middle] = tree.split(prefix, left);
            roots[from] = tree.merge(head, suffix);
            auto [before, after] = tree.split(roots[to], at);
            roots[to] = tree.merge(tree.merge(before, middle), after);

            vector<item> moved(reference[from].begin() + left,
                               reference[from].begin() + right);
            reference[from].erase(reference[from].begin() + left,
                                  reference[from].begin() + right);
            reference[to].insert(reference[to].begin() + at, moved.begin(), moved.end());
        } else {
            nidx_t t = nidx_t(rng() % roots.size());
            auto values = tree.sequence(roots[t]);
            for (nidx_t i = 0; i < values.len(); ++i)
                CHECK(values[i] == reference[t][i].value);
        }

        if (round % 53 == 0) verify();
    }
    verify();

    nfhq<nidx_t> ordered;
    vector<nidx_t> sorted{1, 1, 2, 3, 5, 8};
    nidx_t root = ordered.build(nall(sorted));
    auto [low, high] = ordered.split_by(root, [](nidx_t x) { return x < 3; });
    CHECK((ncollect(ordered.sequence(low)) == vector<nidx_t>{1, 1, 2}));
    CHECK((ncollect(ordered.sequence(high)) == vector<nidx_t>{3, 5, 8}));

    cout << "v4 fhq: destructive roots, parent/rank invariants and split_by passed\n";
}
