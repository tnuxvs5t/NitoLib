#include "common.hpp"

int main() {
    nxorbasis<> basis;
    ntest(basis.ins(3) && basis.ins(5) && !basis.ins(6));
    ntest(basis.len() == 2 && basis.has(6) && basis.max() == 6 && !basis.has(8));

    nprob<double> distribution{1, 2, 3};
    ntest(distribution.sum() == 6);
    auto normalized = distribution.normalized();
    ntest(normalized && abs(normalized->sum() - 1) < 1e-12);
    ntest(abs(normalized->expect([](int index) { return double(index); }) - 4.0 / 3) < 1e-12);
    nprob<double> invalid{1, -1};
    ntest(!invalid.nonnegative() && !invalid.normalized() && invalid.draw(nrng_global, 42) == 42);
    nseed(0x5501U);
    for (int repeat = 0; repeat < 1000; ++repeat)
        ntest(0 <= distribution.draw() && distribution.draw() < distribution.len());

    nnim<unsigned> nim(nvector<unsigned>{3, 4, 5});
    ntest(nim.win() && nim.nim_sum() == (3U ^ 4U ^ 5U));
    auto move = nim.winning();
    ntest(move && move->second < nim.h[move->first]);
    nnim<unsigned> losing(nvector<unsigned>{7, 7});
    ntest(!losing.win() && !losing.winning());

    ngraph_list<> dag(4);
    dag.add(0, 1);
    dag.add(0, 2);
    dag.add(1, 3);
    dag.add(2, 3);
    auto grundy = nsg(dag);
    ntest(grundy && (*grundy)[3] == 0 && (*grundy)[1] == 1 && (*grundy)[2] == 1 &&
          (*grundy)[0] == 0);
    dag.add(3, 0);
    ntest(!nsg_dag(dag));
}
