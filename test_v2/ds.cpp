#include "common.hpp"

struct nconcat_ds {
    static constexpr nlaw laws = nlaw::associative | nlaw::identity;
    string id() const { return {}; }
    string operator()(string a, const string& b) const { return a += b; }
};

struct nmod_group {
    static constexpr nlaw laws =
        nlaw::associative | nlaw::identity | nlaw::inverse | nlaw::commutative;
    int mod;
    int id() const { return 0; }
    int operator()(int a, int b) const { return (a + b) % mod; }
    int inv(int a) const { return a ? mod - a : 0; }
};

template <class O>
concept nstring_fenwick_available = requires { typename nfenwick<string, O>; };

int main() {
    static_assert(!nstring_fenwick_available<nconcat_ds>);

    nvector<long long> values{2, -1, 5, 3};
    nfenwick<long long> fenwick(values);
    ntest(fenwick.prefix(0) == 0 && fenwick.prefix(3) == 6);
    ntest(fenwick.fold(1, 4) == 7 && fenwick.get(2) == 5);
    fenwick.add(1, 10);
    ntest(fenwick.fold(0, 2) == 11);

    nfenwick<int, nmod_group> modular(4, nmod_group{7});
    modular.add(0, 6);
    modular.add(1, 5);
    ntest(modular.prefix(2) == 4 && modular.fold(1, 2) == 5);

    nvector<int> frequencies(10, 0);
    for (int index : {1, 1, 3, 7, 7, 7})
        ++frequencies[index];
    nfenwick<int> order(frequencies);
    ntest(order.lower(1) == 1 && order.lower(2) == 1 && order.lower(3) == 3);
    ntest(order.lower(6) == 7 && order.lower(7) == npos);

    nvector<string> words{"a", "bc", "d", "ef"};
    nseg<string, nconcat_ds> segment(words);
    ntest(segment.fold(0, 4) == "abcdef");
    ntest(segment.fold(1, 3) == "bcd");
    segment.set(2, "XY");
    ntest(segment.fold() == "abcXYef");

    auto reversed = nreverse(words);
    nseg<string, nconcat_ds> reverse_segment(reversed);
    ntest(reverse_segment.fold() == "efdbca");
}
