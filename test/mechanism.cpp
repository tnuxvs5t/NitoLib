#include "common.hpp"

struct nconcat_mechanism {
    string id() const { return {}; }
    string operator()(string a, const string& b) const { return a += b; }
};

int main() {
    nvector<int> a{2, 3, 5, 7};
    auto prefix = nscan(a);
    auto suffix = nsuffix_scan(a);
    ntest((prefix == nvector<int>{0, 2, 5, 10, 17}));
    ntest((suffix == nvector<int>{17, 15, 12, 7, 0}));

    nvector<string> words{"a", "bc", "d"};
    auto left = nscan(words, nconcat_mechanism{});
    auto right = nsuffix_scan(words, nconcat_mechanism{});
    ntest((left == nvector<string>{"", "a", "abc", "abcd"}));
    ntest((right == nvector<string>{"abcd", "bcd", "d", ""}));

    ntest(nfirst_true(0, 100, [](int x) { return 1LL * x * x >= 900; }) == 30);
    ntest(nfirst_true(0, 10, [](int) { return false; }) == 10);
    ntest(nlast_true(0, 100, [](int x) { return 1LL * x * x < 900; }) == 29);

    int x = 1, y = 2;
    nrollback<int> log;
    int root = log.time();
    log.assign(x, 10);
    int branch = log.time();
    log.assign(y, 20);
    log.mutate(x, [](int& value) { value += 5; });
    ntest(x == 15 && y == 20);
    log.rollback(branch);
    ntest(x == 10 && y == 2);
    log.rollback(root);
    ntest(x == 1 && y == 2 && log.empty());
}
