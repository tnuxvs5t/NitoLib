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
    static_assert(!ngroup<nadd<bool>, bool>);
    static_assert(!ngroup<nadd<string>, string>);
    static_assert(!ncommutative_monoid<nadd<string>, string>);
    static_assert(nmonoid<nconcat, string>);
    static_assert(!ncommutative_monoid<nconcat, string>);
    static_assert(ngroup<nmod_add, int>);
    static_assert(nsemiring<nadd<long long>, nmul<long long>, long long>);
    static_assert(!nsemiring<nadd<double>, nmul<double>, double>);
    static_assert(naction<naddsum_action<long long>, long long, long long>);
    static_assert(!naction<naddsum_action<double>, double, double>);

    nvector<string> words{"ka", "ppa", "!"};
    ntest(nfold(words, nconcat{}) == "kappa!");

    nvector<int> values{8, 7, 6};
    ntest(nfold(values, nmod_add{10}) == 1);
    ntest(nhas_law(nmin<int>::laws, nlaw::idempotent));

    ntest(nfold(nvector<int>{1'000'000'000}, nmin<int>{}) == 1'000'000'000);
    ntest(nfold(nvector<int>{-1'000'000'000}, nmax<int>{}) == -1'000'000'000);
    ntest(nmin<int>{}.id() == numeric_limits<int>::max());
    ntest(nmax<int>{}.id() == numeric_limits<int>::lowest());
    ntest(npow(3LL, 5) == 243);
    ntest(npow(3, -1, nmod_add{10}) == 7);
    ntest(npow(3, -3, nmod_add{10}) == 1);
}
