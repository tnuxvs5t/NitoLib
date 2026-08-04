#include "common.hpp"

int main() {
    auto squares = nfunc(nrange(6), [](int value) { return value * value; });
    ntest(squares.len() == 6 && squares.key(4) == 4 && squares(4) == 16 && squares[5] == 25);
    ntest(ncollect(squares) == nvector<int>({0, 1, 4, 9, 16, 25}));

    auto selected = nrestrict(squares, nvector<int>{5, 2, 4});
    ntest(selected.len() == 3 && selected.key(0) == 5 && selected[0] == 25 && selected[1] == 4);
    int checksum = 0;
    nforkv(argument, value, selected)
        checksum += argument * 10 + value;
    ntest(checksum == 5 * 10 + 25 + 2 * 10 + 4 + 4 * 10 + 16);

    constexpr int count = 17, width = 4;
    auto block_of = nfunc(nrange(count), [](int index) { return index / width; });
    auto block_begin = nfunc(nrange((count + width - 1) / width),
                             [](int block) { return block * width; });
    auto begin_of_index = ncompose(block_begin, block_of);
    for (int index = 0; index < count; ++index)
        ntest(begin_of_index(index) == index / width * width);

    nvector<int> sequence{8, 1, 7, 2, 6, 3, 5, 4};
    auto at = nfunc(nrange(sequence.len()), [&](int index) -> int& { return sequence[index]; });
    auto subsequence = nrestrict(at, nvector<int>{1, 3, 5, 7});
    ntest(ncollect(subsequence) == nvector<int>({1, 2, 3, 4}));
    nreverse_inplace(subsequence);
    ntest(sequence == nvector<int>({8, 4, 7, 3, 6, 2, 5, 1}));
    nsort(subsequence);
    ntest(sequence == nvector<int>({8, 1, 7, 2, 6, 3, 5, 4}));

    nvector<int> dp(sequence.len(), 0);
    auto state = nfunc(nrange(dp.len()), [&](int index) -> int& { return dp[index]; });
    auto candidates = nrestrict(state, nvector<int>{0, 2, 4, 6});
    nforkv(index, value, candidates)
        value = sequence[index] + index;
    ntest(dp == nvector<int>({8, 0, 9, 0, 10, 0, 11, 0}));
}
