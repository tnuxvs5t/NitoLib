#include "common.hpp"

struct nconcat_queue {
    static constexpr nlaw laws = nlaw::associative | nlaw::identity;
    string id() const { return {}; }
    string operator()(string a, const string& b) const { return a += b; }
};

int main() {
    nqueue_agg<string, nconcat_queue> queue;
    queue.push("ab");
    queue.push("c");
    ntest(queue.fold() == "abc" && queue.front() == "ab");
    ntest(queue.pop() == "ab");
    queue.push("de");
    ntest(queue.fold() == "cde");
    ntest(queue.pop() == "c" && queue.pop() == "de" && queue.empty());

    ndsu dsu(8);
    dsu.merge(0, 1);
    dsu.merge(2, 3);
    dsu.merge(1, 3);
    dsu.merge(5, 6);
    ntest(dsu.same(0, 2) && dsu.size(3) == 4 && !dsu.same(4, 5));
    auto classes = dsu.partition();
    ntest(classes[0] == classes[3] && classes[0] != classes[4]);

    nrollback_dsu rollback(6);
    int root = rollback.time();
    ntest(rollback.merge(0, 1));
    ntest(rollback.merge(1, 2));
    int branch = rollback.time();
    ntest(rollback.merge(2, 3) && rollback.size(0) == 4);
    rollback.rollback(branch);
    ntest(!rollback.same(0, 3) && rollback.same(0, 2));
    rollback.rollback(root);
    ntest(!rollback.same(0, 1));
}
