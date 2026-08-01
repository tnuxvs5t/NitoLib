#include "common.hpp"

int main() {
    nvector<unsigned> masks;
    nfor(mask, nsubmasks(0b10110U)) masks.push(mask);
    ntest(masks == nvector<unsigned>({22, 20, 18, 16, 6, 4, 2, 0}));
    ntest(nsubmasks(0U).len() == 1);

    nvector<long long> values{1, 2, 3, 4, 5, 6, 7, 8};
    auto original = values;
    nzeta_subset(values);
    ntest(values[7] == 36 && values[5] == 14);
    nmobius_subset(values);
    ntest(values == original);
    nzeta_superset(values);
    ntest(values[0] == 36 && values[2] == 22);
    nmobius_superset(values);
    ntest(values == original);

    mt19937 random(0x52c0bU);
    for (int bits = 0; bits <= 6; ++bits) {
        int n = 1 << bits;
        for (int repeat = 0; repeat < 80; ++repeat) {
            nvector<long long> a(n), b(n);
            for (int i = 0; i < n; ++i) {
                a[i] = int(random() % 15) - 7;
                b[i] = int(random() % 15) - 7;
            }
            nvector<long long> want_or(n), want_and(n), want_xor(n);
            for (int i = 0; i < n; ++i)
                for (int j = 0; j < n; ++j) {
                    want_or[i | j] += a[i] * b[j];
                    want_and[i & j] += a[i] * b[j];
                    want_xor[i ^ j] += a[i] * b[j];
                }
            ntest(nconv_or(a, b) == want_or);
            ntest(nconv_and(a, b) == want_and);
            ntest(nconv_xor(a, b) == want_xor);
        }
    }
}
