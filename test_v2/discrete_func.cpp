#include "common.hpp"

int main() {
    auto squares = nfunc(nrange(6), [](int value) { return value * value; });
    static_assert(ndiscrete<decltype(squares)>);
    ntest(squares.len() == 6 && squares.key(4) == 4 && squares(4) == 16 && squares[5] == 25);
    ntest(ncollect(squares) == nvector<int>({0, 1, 4, 9, 16, 25}));
    ntest(ntabulate(squares) == nvector<int>({0, 1, 4, 9, 16, 25}));
    ntest(ncollect(nkeys(squares)) == nvector<int>({0, 1, 2, 3, 4, 5}));
    ntest((ncollect(nentries(squares)) ==
           nvector<pair<int, int>>({{0, 0}, {1, 1}, {2, 4}, {3, 9}, {4, 16}, {5, 25}})));

    auto labels = nmap_values(squares, [](int value) { return to_string(value) + "!"; });
    ntest(labels.key(3) == 3 && labels(4) == "16!" && labels[5] == "25!");

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

    auto gathered = ngather(at, nvector<int>{7, 1, 7, 3});
    ntest(ncollect(nkeys(gathered)) == nvector<int>({7, 1, 7, 3}));
    ntest(ncollect(gathered) == nvector<int>({4, 1, 4, 2}));
    ntest(gathered.position(2) == 7);
    gathered[0] = 40;
    ntest(sequence[7] == 40 && gathered[2] == 40);

    auto owned = ngather(nfunc(nvector<int>{10, 20, 30}, [](int key) { return key + 1; }),
                         nvector<int>{2, 0});
    ntest(ncollect(nkeys(owned)) == nvector<int>({30, 10}));
    ntest(ntabulate(owned) == nvector<int>({31, 11}));

    nvector<int> dp(sequence.len(), 0);
    auto state = nfunc(nrange(dp.len()), [&](int index) -> int& { return dp[index]; });
    auto candidates = nrestrict(state, nvector<int>{0, 2, 4, 6});
    nforkv(index, value, candidates)
        value = sequence[index] + index;
    ntest(dp == nvector<int>({8, 0, 9, 0, 10, 0, 11, 0}));

    nvector<int> tiles{9, 1, 8, 2, 7, 3, 6, 4};
    auto cell = nfunc(nrange(tiles.len()), [&](int index) -> int& { return tiles[index]; });
    auto blocks = nblocks(cell, 3);
    ntest(blocks.len() == 3);
    nfor(block, blocks)
        nsort(block);
    ntest(tiles == nvector<int>({1, 8, 9, 2, 3, 7, 4, 6}));
    auto tail = nblock(cell, 2, 3);
    ntest(ncollect(nkeys(tail)) == nvector<int>({6, 7}));

    nvector<int> values{3, 1, 4, 2, 5, 0, 6};
    nvector<nvector<int>> predecessor(values.len());
    for (int to = 0; to < values.len(); ++to)
        for (int from = 0; from < to; ++from)
            if (values[from] < values[to])
                predecessor[to].push(from);
    nvector<int> lis(values.len(), 1);
    auto lis_state = nfunc(nrange(lis.len()), [&](int index) -> int& { return lis[index]; });
    for (int to = 0; to < values.len(); ++to) {
        auto previous = ngather(lis_state, predecessor[to]);
        nforkv(from, best, previous)
            nchmax(lis[to], best + 1);
    }
    ntest(lis == nvector<int>({1, 1, 2, 2, 3, 1, 4}));
}
