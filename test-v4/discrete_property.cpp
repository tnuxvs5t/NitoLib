#include "../src-v4/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    vector<nidx_t> values{4, 1, 4, 2, 2, 8};

    auto picked = nselect(nall(values), vector<nidx_t>{5, 0, 5, 3});
    CHECK(picked.len() == 4 && picked[0] == 8 && picked[1] == 4);
    picked[2] = 9;
    CHECK(values[5] == 9);

    auto sliced = nslice(nall(values), 1, 4);
    CHECK((ncollect(sliced) == vector<nidx_t>{1, 4, 2}));

    auto stride = nstride(nall(values), 2);
    CHECK((ncollect(stride) == vector<nidx_t>{4, 4, 2}));
    auto backwards = nstride(nall(values), -2);
    CHECK((ncollect(backwards) == vector<nidx_t>{9, 2, 1}));

    auto prefix = nprefix(nall(values), nidx_t(0));
    auto suffix = nsuffix(nall(values), nidx_t(0));
    CHECK((prefix == vector<nidx_t>{0, 4, 5, 9, 11, 13, 22}));
    CHECK((suffix == vector<nidx_t>{22, 18, 17, 13, 11, 9, 0}));

    auto filtered = nfilter(nall(values), [](nidx_t x) { return x % 2 == 0; });
    CHECK((ncollect(filtered) == vector<nidx_t>{4, 4, 2, 2}));
    auto unique = nunique(nall(values));
    CHECK((ncollect(unique) == vector<nidx_t>{4, 1, 4, 2, 9}));

    auto indexed = nindexed(nall(values));
    CHECK((indexed[2] == tuple{2, 4}));
    auto copied_pairs = ncollect(indexed);
    CHECK((copied_pairs[4] == tuple{4, 2}));

    vector<nidx_t> destination(9, -1);
    ncopy(nall(values), nall(destination));
    for (nidx_t i = 0; i < nidx_t(values.size()); ++i)
        CHECK(values[i] == destination[i]);
    CHECK(destination[8] == -1);
    ntransform(nall(values), nall(destination), [](nidx_t x) { return 2 * x; });
    CHECK(destination[0] == 8 && destination[5] == 18 && destination[8] == -1);
    ntransform(nall(values), nrange(6), nall(destination),
               [](nidx_t x, nidx_t i) { return x + i; });
    CHECK(destination[0] == 4 && destination[5] == 14);
    nfill(nall(destination), nidx_t(3));
    CHECK(ranges::all_of(destination, [](nidx_t x) { return x == 3; }));

    CHECK(naccumulate(nall(values), nidx_t(0)) == 22);
    auto action = neach(nall(values), [sum = nidx_t(0)](nidx_t x) mutable { sum += x; });
    (void)action;
    CHECK(nfind_if(nall(values), [](nidx_t x) { return x > 7; }) == 5);
    CHECK(ncontains(nall(values), 9) && ncount_if(nall(values), [](nidx_t x) { return x == 2; }) == 2);
    CHECK(nall_of(nall(values), [](nidx_t x) { return x > 0; }));
    CHECK(nany_of(nall(values), [](nidx_t x) { return x == 1; }));
    CHECK(nnone_of(nall(values), [](nidx_t x) { return x < 0; }));
    CHECK(nargmin(nall(values)) == 1 && nargmax(nall(values)) == 5);
    vector<nidx_t> bounds_source{1, 2, 2, 2, 4, 9};
    CHECK(nlower(nall(bounds_source), 2) == 1 && nupper(nall(bounds_source), 2) == 4);

    vector<nidx_t> sorted{5, 1, 4, 1, 3};
    auto ordered = norder(nall(sorted));
    CHECK((ncollect(ordered) == vector<nidx_t>{1, 1, 3, 4, 5}));
    nsort(nall(sorted));
    CHECK((sorted == vector<nidx_t>{1, 1, 3, 4, 5}));
    nreverse_inplace(nall(sorted));
    CHECK((sorted == vector<nidx_t>{5, 4, 3, 1, 1}));

    auto blocks = nblocks(nall(values), 3);
    CHECK(blocks.len() == 2 && blocks.key(1) == pair(3, 6));
    auto detached = blocks[1];
    detached[0] = 11;
    CHECK(values[3] == 11);
    auto windows = nwindows(nall(values), 3, 2);
    CHECK(windows.len() == 2 && windows.key(1) == pair(2, 5));
    vector<nidx_t> alternating{0, 1, 0, 1, 0};
    auto runs = nruns(nall(alternating));
    CHECK(runs.len() == 5 && runs.key(3) == pair(3, 4));

    vector<nidx_t> keys{80, 10, 70, 20, 60, 30};
    auto function = nanchors(nall(keys), nall(values),
                             [](nidx_t key) { return (key / 10) % 7; });
    auto function_blocks = nblocks(function, 2);
    CHECK(function_blocks[1].key(0) == 70 && function_blocks[1][0] == function(70));

    mt19937 rng(0xD15C0);
    for (int round = 0; round < 5000; ++round) {
        nidx_t n = nidx_t(rng() % 40);
        vector<nidx_t> input(n);
        for (nidx_t& x : input) x = nidx_t(rng() % 8);

        nidx_t width = 1 + nidx_t(rng() % 12);
        auto partition = nblocks(nall(input), width);
        CHECK(partition.len() == n / width + (n % width != 0));
        for (nidx_t i = 0; i < partition.len(); ++i) {
            nidx_t left = i * width, right = min(n, left + width);
            CHECK(partition.key(i) == pair(left, right));
            for (nidx_t j = left; j < right; ++j)
                CHECK(partition[i][j - left] == input[j]);
        }

        auto runs_random = nruns(nall(input));
        vector<pair<nidx_t, nidx_t>> expected_runs;
        nidx_t left = 0;
        for (nidx_t i = 1; i <= n; ++i) {
            if (i == n || input[i - 1] != input[i]) {
                expected_runs.push_back({left, i});
                left = i;
            }
        }
        CHECK(runs_random.len() == nidx_t(expected_runs.size()));
        for (nidx_t i = 0; i < runs_random.len(); ++i)
            CHECK(runs_random.key(i) == expected_runs[i]);
    }

    cout << "v4 discrete: plans, scans, writes, bounds, chunks and runs passed\n";
}
