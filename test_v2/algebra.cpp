#include "common.hpp"

struct nconcat {
    static constexpr nlaw laws = nlaw::associative | nlaw::identity;
    string id() const { return {}; }
    string operator()(string a, const string& b) const { return a += b; }
};

struct nmod_add {
    static constexpr nlaw laws =
        nlaw::associative | nlaw::identity | nlaw::inverse | nlaw::commutative;
    int mod;
    int id() const { return 0; }
    int operator()(int a, int b) const { return (a + b) % mod; }
    int inv(int a) const { return a ? mod - a : 0; }
};

int main() {
    static_assert(ncommutative_monoid<nadd<long long>, long long>);
    static_assert(!ngroup<nadd<string>, string>);
    static_assert(!ncommutative_monoid<nadd<string>, string>);
    static_assert(nmonoid<nconcat, string>);
    static_assert(!ncommutative_monoid<nconcat, string>);
    static_assert(ngroup<nmod_add, int>);

    nvector<string> words{"ka", "ppa", "!"};
    ntest(nfold(words, nconcat{}) == "kappa!");

    nvector<int> values{8, 7, 6};
    ntest(nfold(values, nmod_add{10}) == 1);
    ntest(nhas_law(nmin<int>::laws, nlaw::idempotent));
}
