#include "common.hpp"

struct nconcat {
    string id() const { return {}; }
    string operator()(string a, const string& b) const { return a += b; }
};

struct nmod_add {
    int mod;
    int id() const { return 0; }
    int operator()(int a, int b) const { return (a + b) % mod; }
    int inv(int a) const { return a ? mod - a : 0; }
};

int main() {
    nvector<string> words{"ka", "ppa", "!"};
    ntest(nfold(words, nconcat{}) == "kappa!");

    nvector<int> values{8, 7, 6};
    ntest(nfold(values, nmod_add{10}) == 1);
    ntest(nfold(nvector<int>{1'000'000'000}, nmin<int>{}) == 1'000'000'000);
    ntest(nfold(nvector<int>{-1'000'000'000}, nmax<int>{}) == -1'000'000'000);
    ntest(nmin<int>{}.id() == numeric_limits<int>::max());
    ntest(nmax<int>{}.id() == numeric_limits<int>::lowest());
    ntest(npow(3LL, 5) == 243);
    ntest(npow(3, -1, nmod_add{10}) == 7);
    ntest(npow(3, -3, nmod_add{10}) == 1);
}
