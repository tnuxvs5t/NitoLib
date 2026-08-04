#include "common.hpp"

int main() {
    npartition partition(nvector<int>{8, 8, 3, 5, 3});
    ntest(partition.classes() == 3 && partition.same(0, 1) && partition.same(2, 4));
    npart_dense dense(nvector<int>{10, 10, 4});
    ntest(dense.classes() == 2 && dense.same(0, 1));
    auto groups = partition.groups();
    ntest(groups == nvector<nvector<int>>({{0, 1}, {2, 4}, {3}}));

    nperm p{2, 0, 1, 4, 3};
    ntest((~p) * p == nperm(5));
    ntest(p.pow(3) == nperm({0, 1, 2, 4, 3}));
    ntest(p.pow(-1) == ~p);
    ntest(p.cycles().classes() == 2);

    nvector<char> values{'a', 'b', 'c', 'd', 'e'};
    ntest(p.pull(values) == nvector<char>({'c', 'a', 'b', 'e', 'd'}));
    ntest(p.push(values) == nvector<char>({'b', 'c', 'a', 'e', 'd'}));

    set<int> stl_values{9, 2, 9, 5};
    auto compression = ncompress_stl(stl_values);
    ntest(compression.len() == 3 && compression.to(2) == 0 && compression.to(9) == 2);

    mt19937 random(0x22f1a1U);
    for (int n = 0; n <= 40; ++n)
        for (int repeat = 0; repeat < 80; ++repeat) {
            nvector<int> mapping(n);
            for (int i = 0; i < n; ++i)
                mapping[i] = i;
            for (int i = n; i-- > 1;)
                swap(mapping[i], mapping[random() % (i + 1)]);
            nperm permutation(mapping);
            for (int exponent = -20; exponent <= 20; ++exponent) {
                auto power = permutation.pow(exponent);
                auto inverse = permutation.pow(-exponent);
                ntest(power * inverse == nperm(n));
            }
        }
}
