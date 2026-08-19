#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    vector<nidx_t> values(7);
    nassign(nall(values), [](nidx_t position) { return position * position; });
    CHECK((values == vector<nidx_t>{0, 1, 4, 9, 16, 25, 36}));
    nfill(nall(values), 9);
    CHECK((values == vector<nidx_t>{9, 9, 9, 9, 9, 9, 9}));
    nfill(nstride(nall(values), 0, 7, 2), -3);
    CHECK((values == vector<nidx_t>{-3, 9, -3, 9, -3, 9, -3}));

    vector<nidx_t> source{2, 4, 6, 8}, destination(6, -1);
    ncopy(nall(source), nreverse(nall(destination)));
    CHECK((destination == vector<nidx_t>{-1, -1, 8, 6, 4, 2}));

    vector<nidx_t> squares(4), sums(4);
    ntransform(nall(source), nall(squares), [](nidx_t value) { return value * value; });
    ntransform(nall(source), nall(squares), nall(sums), plus<>{});
    CHECK((squares == vector<nidx_t>{4, 16, 36, 64}));
    CHECK((sums == vector<nidx_t>{6, 20, 42, 72}));
    ntransform(nall(squares), nall(squares), [](nidx_t value) { return value / 2; });
    CHECK((squares == vector<nidx_t>{2, 8, 18, 32}));

    vector<string> keys{"gamma", "alpha", "beta"};
    vector<nidx_t> payload{3, 1, 2};
    auto function = nfunc_bind(nall(keys), nall(payload));
    nfill(function, 5);
    CHECK((keys == vector<string>{"gamma", "alpha", "beta"}));
    CHECK((payload == vector<nidx_t>{5, 5, 5}));
    ncopy(nrange(3), function);
    CHECK((payload == vector<nidx_t>{0, 1, 2}));
    CHECK(function("beta") == 2);

    auto move_only = nview{
        3, [owner = make_unique<array<nidx_t, 3>>()](nidx_t position) -> nidx_t& {
            return (*owner)[position];
        }};
    nassign(move_only, [](nidx_t position) { return 10 + position; });
    CHECK(move_only[0] == 10 && move_only[1] == 11 && move_only[2] == 12);

    vector<nidx_t> overlap{1, 2, 3, 4};
    ncopy(nslice(nall(overlap), 0, 3), nslice(nall(overlap), 1, 4));
    CHECK((overlap == vector<nidx_t>{1, 1, 1, 1}));

    mt19937 rng(0xC0F17E);
    for (nidx_t round = 0; round < 12000; ++round) {
        nidx_t n = nidx_t(rng() % 60);
        vector<nidx_t> input(n), other(n), permutation(n);
        for (nidx_t& value : input) value = nidx_t(rng() % 2001) - 1000;
        for (nidx_t& value : other) value = nidx_t(rng() % 2001) - 1000;
        iota(permutation.begin(), permutation.end(), 0);
        shuffle(permutation.begin(), permutation.end(), rng);

        vector<nidx_t> actual(n, 0), expected(n, 0);
        nassign(nall(actual), [&](nidx_t position) { return input[position] + position; });
        for (nidx_t i = 0; i < n; ++i) expected[i] = input[i] + i;
        CHECK(actual == expected);

        nfill(nall(actual), 0);
        ranges::fill(expected, 0);
        ncopy(nall(input), nselect(nall(actual), permutation));
        for (nidx_t i = 0; i < n; ++i) expected[permutation[i]] = input[i];
        CHECK(actual == expected);

        nidx_t filled = nidx_t(rng() % 2001) - 1000;
        nidx_t take = n ? nidx_t(rng() % (n + 1)) : 0;
        vector<nidx_t> picked(permutation.begin(), permutation.begin() + take);
        nfill(nselect(nall(actual), picked), filled);
        for (nidx_t position : picked) expected[position] = filled;
        CHECK(actual == expected);

        vector<nidx_t> unary(n), binary(n);
        ntransform(nall(input), nall(unary), [](nidx_t value) { return 3 * value - 7; });
        ntransform(nall(input), nall(other), nall(binary),
                   [](nidx_t left, nidx_t right) { return left - 2 * right; });
        for (nidx_t i = 0; i < n; ++i) {
            CHECK(unary[i] == 3 * input[i] - 7);
            CHECK(binary[i] == input[i] - 2 * other[i]);
        }
    }
}
