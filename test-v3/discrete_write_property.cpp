#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    vector<int> values(7);
    nassign(nall(values), [](int position) { return position * position; });
    CHECK((values == vector<int>{0, 1, 4, 9, 16, 25, 36}));
    nfill(nall(values), 9);
    CHECK((values == vector<int>{9, 9, 9, 9, 9, 9, 9}));
    nfill(nstride(nall(values), 0, 7, 2), -3);
    CHECK((values == vector<int>{-3, 9, -3, 9, -3, 9, -3}));

    vector<int> source{2, 4, 6, 8}, destination(6, -1);
    ncopy(nall(source), nreverse(nall(destination)));
    CHECK((destination == vector<int>{-1, -1, 8, 6, 4, 2}));

    vector<int> squares(4), sums(4);
    ntransform(nall(source), nall(squares), [](int value) { return value * value; });
    ntransform(nall(source), nall(squares), nall(sums), plus<>{});
    CHECK((squares == vector<int>{4, 16, 36, 64}));
    CHECK((sums == vector<int>{6, 20, 42, 72}));
    ntransform(nall(squares), nall(squares), [](int value) { return value / 2; });
    CHECK((squares == vector<int>{2, 8, 18, 32}));

    vector<string> keys{"gamma", "alpha", "beta"};
    vector<int> payload{3, 1, 2};
    auto function = nfunc_bind(nall(keys), nall(payload));
    nfill(function, 5);
    CHECK((keys == vector<string>{"gamma", "alpha", "beta"}));
    CHECK((payload == vector<int>{5, 5, 5}));
    ncopy(nrange(3), function);
    CHECK((payload == vector<int>{0, 1, 2}));
    CHECK(function("beta") == 2);

    auto move_only = nview{
        3, [owner = make_unique<array<int, 3>>()](int position) -> int& {
            return (*owner)[position];
        }};
    nassign(move_only, [](int position) { return 10 + position; });
    CHECK(move_only[0] == 10 && move_only[1] == 11 && move_only[2] == 12);

    vector<int> overlap{1, 2, 3, 4};
    ncopy(nslice(nall(overlap), 0, 3), nslice(nall(overlap), 1, 4));
    CHECK((overlap == vector<int>{1, 1, 1, 1}));

    mt19937 rng(0xC0F17E);
    for (int round = 0; round < 12000; ++round) {
        int n = int(rng() % 60);
        vector<int> input(n), other(n), permutation(n);
        for (int& value : input) value = int(rng() % 2001) - 1000;
        for (int& value : other) value = int(rng() % 2001) - 1000;
        iota(permutation.begin(), permutation.end(), 0);
        shuffle(permutation.begin(), permutation.end(), rng);

        vector<int> actual(n, 0), expected(n, 0);
        nassign(nall(actual), [&](int position) { return input[position] + position; });
        for (int i = 0; i < n; ++i) expected[i] = input[i] + i;
        CHECK(actual == expected);

        nfill(nall(actual), 0);
        ranges::fill(expected, 0);
        ncopy(nall(input), nselect(nall(actual), permutation));
        for (int i = 0; i < n; ++i) expected[permutation[i]] = input[i];
        CHECK(actual == expected);

        int filled = int(rng() % 2001) - 1000;
        int take = n ? int(rng() % (n + 1)) : 0;
        vector<int> picked(permutation.begin(), permutation.begin() + take);
        nfill(nselect(nall(actual), picked), filled);
        for (int position : picked) expected[position] = filled;
        CHECK(actual == expected);

        vector<int> unary(n), binary(n);
        ntransform(nall(input), nall(unary), [](int value) { return 3 * value - 7; });
        ntransform(nall(input), nall(other), nall(binary),
                   [](int left, int right) { return left - 2 * right; });
        for (int i = 0; i < n; ++i) {
            CHECK(unary[i] == 3 * input[i] - 7);
            CHECK(binary[i] == input[i] - 2 * other[i]);
        }
    }
}
