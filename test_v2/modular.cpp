#include "common.hpp"

int main() {
    using mint = nmodint<1000000007>;
    static_assert(ncommutative_monoid<nadd<mint>, mint>);
    static_assert(naction<naddsum_action<mint>, mint, mint>);
    mint a = -2, b = 5;
    ntest(a.val() == 1000000005 && (a + b).val() == 3);
    ntest((b - a).val() == 7 && (a * b).val() == 999999997);
    ntest(b.pow(10).val() == 9765625);
    ntest((b * b.inverse().val()).val() == 1);
    ntest((mint(10) / mint(2)).val() == 5);
    nfenwick<mint> fenwick(nvector<mint>{1, 2, 3});
    ntest(fenwick.fold(0, 3).val() == 6);

    using composite = nmodint<12>;
    ntest(!composite(6).inverse());
    ntest(composite(5).inverse().val().val() == 5);

    ncomb<mint> combinations(1000);
    ntest(combinations.choose(5, 2).val() == 10);
    ntest(combinations.permute(5, 2).val() == 20);
    ntest(combinations.choose(5, -1).val() == 0 && combinations.choose(5, 8).val() == 0);
    ntest(combinations.choose(1000, 500).val() == 159835829);

    using compatible = nmod<998244353>;
    compatible x = -3, y = 5;
    ntest(uint64_t(x + y) == 2 && uint64_t(x * y) == 998244338);
    ntest(!compatible(0).tryinv() && compatible(0).inv(compatible(7)) == compatible(7));

    ndmod<0>::setmod(17);
    ndmod<1>::setmod(12);
    ndmod<0> dynamic_prime = 20;
    ndmod<1> dynamic_composite = 20;
    ntest(uint64_t(dynamic_prime) == 3 && uint64_t(dynamic_composite) == 8);
    ntest(uint64_t(dynamic_prime / ndmod<0>(3)) == 1);
    ntest(!dynamic_composite.tryinv());
}
