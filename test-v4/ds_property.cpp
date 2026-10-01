#include "../src-v4/view.hpp"
#include "../src-v4/ds.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

struct min_op {
    nidx_t id() const { return numeric_limits<nidx_t>::max(); }
    nidx_t operator()(nidx_t a, nidx_t b) const { return min(a, b); }
};

int main() {
    vector<nidx_t> values{1, 2, 3, 4, 5};
    nfenwick tree(nall(values));
    CHECK(tree.len() == 5 && tree.prefix(4) == 10 && tree.fold(1, 4) == 9);
    tree.add(2, 7);
    CHECK(tree.get(2) == 10 && tree.fold(0, tree.len()) == 22);
    tree.set(2, 3);
    CHECK(tree.get(2) == 3 && tree.lower_bound(7) == 3 && tree.upper_bound(7) == 3);

    ndsu dsu(7);
    CHECK(dsu.merge(0, 1) == 0 && dsu.merge(1, 2) == 0);
    CHECK(dsu.same(0, 2) && dsu.size(1) == 3 && !dsu.same(0, 3));
    CHECK(dsu.merge(0, 2) == 0);

    npotential_dsu<nidx_t> weighted(4);
    CHECK(weighted.merge(0, 1, 5));
    CHECK(weighted.merge(1, 2, 3));
    CHECK(weighted.difference(0, 2).value() == 8);
    CHECK(weighted.difference(0, 3) == nullopt);
    CHECK(!weighted.merge(0, 2, 7) && weighted.merge(0, 2, 8));

    nrollback_dsu rollback(5);
    CHECK(rollback.merge(0, 1) && rollback.merge(1, 2));
    nidx_t checkpoint = rollback.time();
    CHECK(rollback.merge(3, 4) && rollback.same(3, 4));
    rollback.rollback(checkpoint);
    CHECK(!rollback.same(3, 4) && rollback.same(0, 2));

    nqueue_agg<string, concat> queue;
    queue.push("a"); queue.push("b"); queue.push("c");
    CHECK(queue.fold() == "abc" && queue.front() == "a");
    queue.pop();
    CHECK(queue.fold() == "bc" && queue.front() == "b");

    ndeque_agg<string, concat> deque;
    deque.push_back("b"); deque.push_front("a"); deque.push_back("c");
    CHECK(deque.len() == 3 && deque[0] == "a" && deque[2] == "c");
    CHECK(deque.fold() == "abc" && deque.front() == "a" && deque.back() == "c");
    deque.pop_front(); deque.push_front("z"); deque.pop_back();
    CHECK(deque.fold() == "zb");

    vector<nidx_t> input{7, 2, 5, 2, 9, 1};
    nsparse_table sparse(nall(input), min_op{});
    CHECK(sparse.fold(1, 5) == 2 && sparse.fold(0, 6) == 1);
    CHECK(sparse.get(3) == 2 && sparse.len() == 6);

    mt19937 rng(0xD5U);
    for (int round = 0; round < 3000; ++round) {
        nidx_t n = nidx_t(rng() % 50);
        vector<nidx_t> a(n);
        for (nidx_t& x : a) x = nidx_t(rng() % 20);
        nfenwick fenwick(nall(a));
        for (nidx_t left = 0; left <= n; ++left)
            for (nidx_t right = left; right <= n; ++right) {
                nidx_t sum = accumulate(a.begin() + left, a.begin() + right, nidx_t(0));
                CHECK(fenwick.fold(left, right) == sum);
            }
        ndsu components(n);
        vector<nidx_t> parent(n);
        iota(parent.begin(), parent.end(), 0);
        for (nidx_t i = 1; i < n; ++i) {
            nidx_t j = nidx_t(rng() % i);
            components.merge(i, j);
            for (nidx_t& p : parent) if (p == i) p = j;
        }
        (void)parent;
    }

    cout << "v4 ds: Fenwick, DSU, potentials, rollback and ordered aggregates passed\n";
}
