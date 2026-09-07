#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    vector<nidx_t> values{1, 1, 2, 2, 2, 3, 1, 1};
    auto unique = nunique(nall(values));
    CHECK((ncollect(unique) == vector<nidx_t>{1, 2, 3, 1}));
    unique[3] = 7;
    CHECK(values[6] == 7 && values[7] == 1);

    vector<nidx_t> empty;
    CHECK(nunique(nall(empty)).len() == 0);
    vector<nidx_t> same(9, 4);
    CHECK((ncollect(nunique(nall(same))) == vector<nidx_t>{4}));
    vector<nidx_t> alternating{0, 1, 0, 1, 0};
    CHECK(nunique(nall(alternating)).len() == nidx_t(alternating.size()));

    vector<nidx_t> chained{1, 2, 3, 7, 8, 10};
    auto tolerant = nunique(nall(chained), [](nidx_t previous, nidx_t current) {
        return abs(previous - current) <= 1;
    });
    CHECK((ncollect(tolerant) == vector<nidx_t>{1, 7, 10}));

    vector<nidx_t> keys{80, 10, 70, 20, 60, 30, 50, 40};
    vector<nidx_t> payload{1, 1, 2, 2, 2, 3, 1, 1};
    array<nidx_t, 81> locate{};
    for (nidx_t i = 0; i < nidx_t(keys.size()); ++i) locate[keys[i]] = i;
    auto function = nanchors(nall(keys), nall(payload),
                             [&](nidx_t key) { return locate[key]; });
    auto function_unique = nunique(function);
    CHECK(function_unique.len() == 4);
    CHECK((ncollect(nkeys(function_unique)) == vector<nidx_t>{80, 70, 30, 50}));
    CHECK((ncollect(nvalues(function_unique)) == vector<nidx_t>{1, 2, 3, 1}));
    for (nidx_t i = 0; i < function_unique.len(); ++i)
        CHECK(function_unique[i] == function(function_unique.key(i)));

    auto move_only = ntabulate(
        6, [owner = make_unique<array<nidx_t, 6>>(array<nidx_t, 6>{1, 1, 3, 3, 2, 2})]
               (nidx_t i) -> nidx_t& { return (*owner)[i]; });
    auto move_unique = nunique(move(move_only));
    CHECK((ncollect(move(move_unique)) == vector<nidx_t>{1, 3, 2}));

    mt19937 rng(0xA11CE);
    for (nidx_t round = 0; round < 15000; ++round) {
        nidx_t n = nidx_t(rng() % 60);
        vector<nidx_t> input(n);
        for (nidx_t& value : input) value = nidx_t(rng() % 15) - 7;
        nidx_t threshold = nidx_t(rng() % 5);

        vector<nidx_t> expected;
        for (nidx_t i = 0; i < n; ++i)
            if (!i || abs(input[i - 1] - input[i]) > threshold)
                expected.push_back(input[i]);

        auto actual = nunique(nall(input), [=](nidx_t previous, nidx_t current) {
            return abs(previous - current) <= threshold;
        });
        CHECK(ncollect(actual) == expected);
    }
}
