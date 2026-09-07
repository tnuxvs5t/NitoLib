#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    vector<nidx_t> values{5, 2, 4, 2, 9};
    int calls = 0;
    CHECK(ncontains(nall(values), 4, [&calls](nidx_t value, nidx_t target) {
        ++calls;
        return value == target;
    }));
    CHECK(calls == 3);
    CHECK(!ncontains(nall(values), 6));

    vector<pair<nidx_t, nidx_t>> records{{4, 10}, {1, 20}, {4, 30}};
    CHECK(ncontains(nall(records), 1, equal_to<>{}, &pair<nidx_t, nidx_t>::first));
    CHECK(!ncontains(nall(records), 2, equal_to<>{}, &pair<nidx_t, nidx_t>::first));

    vector<nidx_t> empty;
    CHECK(!ncontains(nall(empty), 0));
    auto function = nanchors(nall(values));
    CHECK(ncontains(function, 9) && !ncontains(function, 6));

    auto move_only = ntabulate(
        5, [owner = make_unique<array<nidx_t, 5>>(array<nidx_t, 5>{8, 3, 1, 7, 5})]
               (nidx_t i) -> nidx_t& { return (*owner)[i]; });
    CHECK(ncontains(move(move_only), 7));

    mt19937 rng(0xC07A1);
    for (nidx_t round = 0; round < 15000; ++round) {
        nidx_t n = nidx_t(rng() % 70);
        vector<nidx_t> input(n);
        for (nidx_t& value : input) value = nidx_t(rng() % 41) - 20;
        nidx_t target = nidx_t(rng() % 41) - 20;

        bool expected = ranges::find(input, target) != input.end();
        CHECK(ncontains(nall(input), target) == expected);

        nidx_t radius = nidx_t(rng() % 5);
        bool near_expected = false;
        for (nidx_t value : input) near_expected |= abs(value - target) <= radius;
        CHECK(ncontains(nall(input), target,
                       [radius](nidx_t value, nidx_t wanted) {
                           return abs(value - wanted) <= radius;
                       }) == near_expected);

        vector<pair<nidx_t, nidx_t>> projected(n);
        for (nidx_t i = 0; i < n; ++i) projected[i] = {input[i], i};
        CHECK(ncontains(nall(projected), target, equal_to<>{},
                       &pair<nidx_t, nidx_t>::first) == expected);
    }
}
