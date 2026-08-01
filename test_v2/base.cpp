#include "common.hpp"

int main() {
    static_assert(nversion == 20000);
#ifdef NITORI_TEST_UNSAFE
    static_assert(nunsafe);
#else
    static_assert(!nunsafe);
#endif

    nmaybe<int> none, some = 7;
    ntest(!none && some && some.val() == 7);
    ntest(none.val(3) == 3 && some.val(3) == 7);

    int x = 9;
    ntest(nchmin(x, 4) && x == 4);
    ntest(!nchmin(x, 8) && nchmax(x, 12) && x == 12);

    array<int, 4> a{};
    ntest(nlen(a) == 4);
    ntest(ninf<int> > 1000000 && nninf<int> < -1000000);
}
