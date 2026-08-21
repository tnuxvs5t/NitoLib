#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    vector<nidx_t> values{1, 1, 2, 2, 2, 3, 1, 1};
    auto blocks = nblocks(nall(values), 3);
    CHECK(blocks.len() == 3);
    CHECK(blocks.key(0) == pair(0, 3) && blocks.key(2) == pair(6, 8));
    CHECK((ncollect(blocks[0]) == vector<nidx_t>{1, 1, 2}));
    CHECK((ncollect(blocks(pair{3, 6})) == vector<nidx_t>{2, 2, 3}));
    auto detached = nblocks(nall(values), 3)[2];
    detached[0] = 7;
    CHECK(values[6] == 7 && ncollect(nblock(nall(values), 3, 2)) == vector<nidx_t>({7, 1}));

    auto windows = nwindows(nall(values), 3, 2);
    CHECK(windows.len() == 3);
    CHECK(windows.key(1) == pair(2, 5));
    CHECK((ncollect(windows[1]) == vector<nidx_t>{2, 2, 2}));
    CHECK(nwindows(nall(values), 20).len() == 0);

    vector<nidx_t> semantic_keys{80, 10, 70, 20, 60, 30, 50, 40};
    array<nidx_t, 81> locate{};
    for (nidx_t i = 0; i < nidx_t(semantic_keys.size()); ++i) locate[semantic_keys[i]] = i;
    auto function = nanchors(nall(semantic_keys), nall(values),
                               [&](nidx_t key) { return locate[key]; });
    auto function_blocks = nblocks(function, 3);
    auto inner = function_blocks[1];
    CHECK(inner.len() == 3 && inner.key(0) == 20 && inner.key(2) == 30);
    CHECK(inner[1] == function(60));
    inner[0] = 12;
    CHECK(function(20) == 12);

    vector<nidx_t> empty;
    CHECK(nruns(nall(empty)).len() == 0);
    vector<nidx_t> same(10, 4);
    auto one_run = nruns(nall(same));
    CHECK(one_run.len() == 1 && one_run.key(0) == pair(0, 10));
    vector<nidx_t> alternating{0, 1, 0, 1, 0};
    auto five_runs = nruns(nall(alternating));
    CHECK(five_runs.len() == 5);
    for (nidx_t i = 0; i < five_runs.len(); ++i)
        CHECK(five_runs.key(i) == pair(i, i + 1) && five_runs[i][0] == alternating[i]);

    mt19937 rng(0xC8A5);
    for (nidx_t round = 0; round < 15000; ++round) {
        nidx_t n = nidx_t(rng() % 55);
        vector<nidx_t> input(n);
        for (nidx_t& value : input) value = nidx_t(rng() % 8);

        nidx_t width = 1 + nidx_t(rng() % 12);
        auto partition = nblocks(nall(input), width);
        nidx_t expected_blocks = n / width + (n % width != 0);
        CHECK(partition.len() == expected_blocks);
        for (nidx_t block = 0; block < partition.len(); ++block) {
            nidx_t left = block * width, right = min(n, left + width);
            CHECK(partition.key(block) == pair(left, right));
            auto chunk = partition[block];
            CHECK(chunk.len() == right - left);
            for (nidx_t i = left; i < right; ++i) CHECK(chunk[i - left] == input[i]);
        }

        nidx_t window_width = 1 + nidx_t(rng() % 12), step = 1 + nidx_t(rng() % 8);
        auto window_list = nwindows(nall(input), window_width, step);
        nidx_t expected_windows = window_width <= n ? 1 + (n - window_width) / step : 0;
        CHECK(window_list.len() == expected_windows);
        for (nidx_t window = 0; window < window_list.len(); ++window) {
            nidx_t left = window * step, right = left + window_width;
            CHECK(window_list.key(window) == pair(left, right));
            for (nidx_t i = left; i < right; ++i) CHECK(window_list[window][i - left] == input[i]);
        }

        vector<pair<nidx_t, nidx_t>> expected_runs;
        nidx_t left = 0;
        for (nidx_t i = 1; i <= n; ++i)
            if (i == n || input[i - 1] != input[i])
                expected_runs.push_back({left, i}), left = i;
        auto runs = nruns(nall(input));
        CHECK(runs.len() == nidx_t(expected_runs.size()));
        for (nidx_t run = 0; run < runs.len(); ++run) {
            CHECK(runs.key(run) == expected_runs[run]);
            auto [begin, end] = expected_runs[run];
            auto chunk = runs[run];
            CHECK(chunk.len() == end - begin);
            for (nidx_t i = begin; i < end; ++i) CHECK(chunk[i - begin] == input[i]);
        }

        nidx_t threshold = nidx_t(rng() % 4);
        auto tolerant = nruns(nall(input), [=](nidx_t left_value, nidx_t right_value) {
            return abs(left_value - right_value) <= threshold;
        });
        nidx_t count = n ? 1 : 0;
        for (nidx_t i = 1; i < n; ++i) count += abs(input[i - 1] - input[i]) > threshold;
        CHECK(tolerant.len() == count);
    }
}
