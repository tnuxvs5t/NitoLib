#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    vector<nidx_t> staircase{1, 2, 3, 4};
    auto relative_to_first = nruns(nall(staircase), [&](nidx_t left, nidx_t right) {
        return staircase[right - 1] - staircase[left] <= 1;
    });
    CHECK(relative_to_first.len() == 2);
    CHECK((relative_to_first.key(0) == pair{0, 2}));
    CHECK(nruns(nall(staircase), [&](nidx_t left, nidx_t right) {
        return right - left == 1 || staircase[right - 1] - staircase[right - 2] <= 1;
    }).len() == 1);
    vector<pair<nidx_t, nidx_t>> calls_seen;
    auto fixed = nruns(nall(staircase), [&](nidx_t left, nidx_t right) {
        calls_seen.emplace_back(left, right);
        return right - left <= 2;
    });
    CHECK(fixed.len() == 2);
    CHECK((calls_seen == vector<pair<nidx_t, nidx_t>>{{0, 1}, {0, 2}, {0, 3}, {2, 3}, {2, 4}}));

    mt19937 rng(811275);
    for (nidx_t trial = 0; trial < 4000; ++trial) {
        nidx_t n = nidx_t(rng() % 80), cap = 1 + nidx_t(rng() % 30);
        nidx_t max_width = 1 + nidx_t(rng() % 10), tolerance = nidx_t(rng() % 8);
        vector<nidx_t> values(n);
        for (auto& value : values) value = 1 + nidx_t(rng() % cap);
        // Brute rescans each entire candidate block; implementation callback keeps running sum.
        vector<pair<nidx_t, nidx_t>> expected;
        for (nidx_t left = 0; left < n;) {
            nidx_t right = left + 1;
            while (right < n && right - left < max_width &&
                   accumulate(values.begin() + left, values.begin() + right + 1, nidx_t(0)) <= cap)
                ++right;
            expected.emplace_back(left, right);
            left = right;
        }
        nidx_t calls = 0, current_left = -1, sum = 0, previous_right = 0;
        auto function = nfunc{nrange(100, 100 + n), [p = &values](nidx_t key) -> nidx_t& {
            return (*p)[key - 100];
        }};
        auto result = nruns(move(function), [&, state = make_unique<nidx_t>(0)]
                            (nidx_t left, nidx_t right) mutable {
            ++calls;
            ++*state;
            if (current_left != left) {
                CHECK(right == left + 1);
                current_left = left;
                sum = 0;
            } else CHECK(right == previous_right + 1);
            previous_right = right;
            sum += values[right - 1];
            return right - left <= max_width && sum <= cap;
        });
        CHECK(calls == (n ? n + nidx_t(expected.size()) - 1 : 0));
        CHECK(result.len() == nidx_t(expected.size()));
        for (nidx_t i = 0; i < result.len(); ++i) {
            CHECK(result.key(i) == expected[i]);
            auto [left, right] = expected[i];
            auto child = result[i];
            CHECK(child.len() == right - left && child.key(0) == left + 100);
            for (nidx_t j = left; j < right; ++j) CHECK(child(j + 100) == values[j]);
        }
        auto extremes = nruns(nall(values), [&, start = nidx_t(-1), low = nidx_t(0), high = nidx_t(0)]
                              (nidx_t left, nidx_t right) mutable {
            nidx_t value = values[right - 1];
            if (start != left) start = left, low = value, high = value;
            else low = min(low, value), high = max(high, value);
            return high - low <= tolerance;
        });
        nidx_t covered = 0;
        for (nidx_t i = 0; i < extremes.len(); ++i) {
            auto [left, right] = extremes.key(i);
            CHECK(left == covered && left < right);
            vector<nidx_t> sorted(values.begin() + left, values.begin() + right);
            sort(sorted.begin(), sorted.end());
            CHECK(sorted.back() - sorted.front() <= tolerance);
            if (right < n)
                CHECK(max(sorted.back(), values[right]) - min(sorted.front(), values[right]) > tolerance);
            covered = right;
        }
        CHECK(covered == n);
    }
    auto detached = [] {
        auto source = ntabulate(7, [token = make_unique<nidx_t>(3)](nidx_t i) { return i + *token; });
        return nruns(move(source), [](nidx_t left, nidx_t right) {
            return right - left <= 3;
        })[1];
    }();
    CHECK(detached.len() == 3 && detached[0] == 6 && detached[2] == 8);
}
