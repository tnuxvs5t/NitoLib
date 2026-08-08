#include "common.hpp"

struct nconcat_persistent {
    string id() const { return {}; }
    string operator()(string a, const string& b) const { return a += b; }
};

int main() {
    nvector<int> initial{1, 2, 3, 4};
    npersistent_seg<int> tree(initial);
    int version1 = tree.set(0, 1, 20);
    int version2 = tree.set(0, 2, 30);
    int version3 = tree.set(version1, 3, 40);

    ntest(tree.fold(0) == 10);
    ntest(tree.fold(version1) == 28 && tree.get(version1, 1) == 20);
    ntest(tree.fold(version2) == 37 && tree.get(version2, 1) == 2);
    ntest(tree.fold(version3) == 64 && tree.get(version3, 2) == 3);
    ntest(tree.fork(0) == 4 && tree.fold(4) == 10);

    nvector<string> words{"a", "b", "c"};
    npersistent_seg<string, nconcat_persistent> strings(words);
    int changed = strings.set(0, 1, "XY");
    ntest(strings.fold(0) == "abc");
    ntest(strings.fold(changed) == "aXYc");
    ntest(strings.fold(changed, 1, 3) == "XYc");
}
