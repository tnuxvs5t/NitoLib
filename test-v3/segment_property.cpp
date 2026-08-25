#include "../src-v3/segment.hpp"
#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

struct affine {
    long long a = 1, b = 0;
};

struct affine_sum {
    affine tag_id() const { return {}; }
    affine compose(const affine& newer, const affine& older) const {
        return {newer.a * older.a, newer.a * older.b + newer.b};
    }
    long long apply(long long sum, const affine& tag, nidx_t length) const {
        return tag.a * sum + tag.b * length;
    }
};

struct assign_string {
    char tag_id() const { return 0; }
    char compose(char newer, char) const { return newer; }
    string apply(string, char tag, nidx_t length) const { return string(length, tag); }
};

int main() {
    using pair_value = pair<long long, int>;
    using tuple_value = tuple<int, pair<long long, int>, double>;
    const auto pair_high = pair_value{numeric_limits<long long>::max(),
                                      numeric_limits<int>::max()};
    const auto pair_low = pair_value{numeric_limits<long long>::lowest(),
                                     numeric_limits<int>::lowest()};
    CHECK((nadd<pair_value>{}.id() == pair_value{}));
    CHECK((nadd<pair_value>{}({2, -5}, {7, 3}) == pair_value{9, -2}));
    CHECK((nmin<pair_value>{}.id() == pair_high));
    CHECK((nmax<pair_value>{}.id() == pair_low));

    const tuple_value tuple_zero{};
    const tuple_value tuple_high{numeric_limits<int>::max(), pair_high,
                                 numeric_limits<double>::infinity()};
    const tuple_value tuple_low{numeric_limits<int>::lowest(), pair_low,
                                -numeric_limits<double>::infinity()};
    CHECK((nadd<tuple_value>{}.id() == tuple_zero));
    CHECK((nadd<tuple_value>{}({1, {2, 3}, 4.5}, {5, {6, 7}, 8.5}) ==
           tuple_value{6, {8, 10}, 13.0}));
    CHECK((nmin<tuple_value>{}.id() == tuple_high));
    CHECK((nmax<tuple_value>{}.id() == tuple_low));

    vector<pair_value> pair_boundary{{numeric_limits<long long>::max(), 7}};
    nseg<pair_value, nmin<pair_value>> pair_min_tree(nall(pair_boundary));
    CHECK((pair_min_tree.fold(0, 1) == pair_boundary[0]));
    pair_boundary[0] = {numeric_limits<long long>::lowest(), -7};
    nseg<pair_value, nmax<pair_value>> pair_max_tree(nall(pair_boundary));
    CHECK((pair_max_tree.fold(0, 1) == pair_boundary[0]));

    vector<long long> extrema{-8, 4, -3, 12, 1};
    nseg<long long, nmin<long long>> min_tree(nall(extrema));
    nseg<long long, nmax<long long>> max_tree(nall(extrema));
    CHECK(min_tree.fold() == -8 && min_tree.fold(1, 4) == -3);
    CHECK(max_tree.fold() == 12 && max_tree.fold(0, 3) == 4);
    CHECK(naccumulate(nall(extrema), nmin<long long>{}.id(), nmin<long long>{}) == -8);
    CHECK(naccumulate(nall(extrema), nmax<long long>{}.id(), nmax<long long>{}) == 12);
    min_tree.set(1, -20);
    max_tree.set(3, 30);
    CHECK(min_tree.fold() == -20 && max_tree.fold() == 30);
    nseg<long long, nmin<long long>> empty_min;
    nseg<long long, nmax<long long>> empty_max;
    CHECK(empty_min.fold() == numeric_limits<long long>::max());
    CHECK(empty_max.fold() == numeric_limits<long long>::lowest());

    mt19937 rng(0x5E6);
    const double infinity = numeric_limits<double>::infinity();
    vector<double> positive_infinity(3, infinity);
    vector<double> negative_infinity(3, -infinity);
    nseg<double, nmin<double>> infinity_min(nall(positive_infinity));
    nseg<double, nmax<double>> infinity_max(nall(negative_infinity));
    CHECK(infinity_min.fold() == infinity && infinity_min.fold(1, 1) == infinity);
    CHECK(infinity_max.fold() == -infinity && infinity_max.fold(2, 2) == -infinity);
    CHECK((nseg<double, nmin<double>>{}.fold() == infinity));
    CHECK((nseg<double, nmax<double>>{}.fold() == -infinity));

    for (nidx_t round = 0; round < 3000; ++round) {
        nidx_t n = nidx_t(rng() % 65);
        vector<double> values(n);
        for (double& value : values) {
            nidx_t kind = nidx_t(rng() % 17);
            value = kind == 0 ? infinity : kind == 1 ? -infinity
                                                    : double(nidx_t(rng() % 201) - 100);
        }
        nseg<double, nmin<double>> floating_min(nall(values));
        nseg<double, nmax<double>> floating_max(nall(values));
        for (nidx_t step = 0; step < 100; ++step) {
            if (n && rng() % 4 == 0) {
                nidx_t position = nidx_t(rng() % n);
                nidx_t kind = nidx_t(rng() % 17);
                values[position] = kind == 0 ? infinity : kind == 1 ? -infinity
                                                                   : double(nidx_t(rng() % 201) - 100);
                floating_min.set(position, values[position]);
                floating_max.set(position, values[position]);
            } else {
                nidx_t left = n ? nidx_t(rng() % (n + 1)) : 0;
                nidx_t right = left + nidx_t(rng() % (n - left + 1));
                double expected_min = infinity, expected_max = -infinity;
                for (nidx_t i = left; i < right; ++i) {
                    if (values[i] < expected_min) expected_min = values[i];
                    if (expected_max < values[i]) expected_max = values[i];
                }
                CHECK(floating_min.fold(left, right) == expected_min);
                CHECK(floating_max.fold(left, right) == expected_max);
            }
        }
    }

    for (nidx_t round = 0; round < 5000; ++round) {
        nidx_t n = nidx_t(rng() % 45);
        vector<string> values(n);
        for (string& value : values) value = char('a' + rng() % 5);
        nseg tree(nall(values), concat{});
        for (nidx_t step = 0; step < 100; ++step) {
            if (n && rng() % 3 == 0) {
                nidx_t position = nidx_t(rng() % n);
                values[position] = char('a' + rng() % 5);
                tree.set(position, values[position]);
            } else {
                nidx_t left = n ? nidx_t(rng() % (n + 1)) : 0;
                nidx_t right = left + nidx_t(rng() % (n - left + 1));
                string expected;
                for (nidx_t i = left; i < right; ++i) expected += values[i];
                CHECK(tree.fold(left, right) == expected);
            }
        }
        CHECK(tree.fold() == accumulate(values.begin(), values.end(), string{}));

        vector<string> other(n);
        for (string& value : other) value = char('x' + rng() % 3);
        nseg second(nall(other), concat{});
        tree.pointwise(second);
        string expected;
        for (nidx_t i = 0; i < n; ++i) expected += values[i] + other[i];
        CHECK(tree.fold() == expected);
    }

    for (nidx_t round = 0; round < 5000; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 70);
        vector<long long> values(n);
        for (long long& value : values) value = nidx_t(rng() % 101) - 50;
        nlazyseg<long long, affine, nadd<long long>, affine_sum>
            tree(nall(values), {}, {});
        for (nidx_t step = 0; step < 200; ++step) {
            nidx_t operation = nidx_t(rng() % 4);
            if (operation == 0) {
                nidx_t left = nidx_t(rng() % (n + 1));
                nidx_t right = left + nidx_t(rng() % (n - left + 1));
                affine tag{nidx_t(rng() % 3) - 1, nidx_t(rng() % 11) - 5};
                tree.apply(left, right, tag);
                for (nidx_t i = left; i < right; ++i) values[i] = tag.a * values[i] + tag.b;
            } else if (operation == 1) {
                nidx_t position = nidx_t(rng() % n);
                values[position] = nidx_t(rng() % 101) - 50;
                tree.set(position, values[position]);
            } else {
                nidx_t left = nidx_t(rng() % (n + 1));
                nidx_t right = left + nidx_t(rng() % (n - left + 1));
                CHECK(tree.fold(left, right) ==
                      accumulate(values.begin() + left, values.begin() + right, 0LL));
            }
        }
        CHECK(tree.fold() == accumulate(values.begin(), values.end(), 0LL));
    }

    for (nidx_t round = 0; round < 2000; ++round) {
        nidx_t n = 1 + nidx_t(rng() % 60);
        vector<string> values(n);
        for (string& value : values) value = char('a' + rng() % 5);
        nlazyseg<string, char, concat, assign_string> tree(nall(values), {}, {});
        for (nidx_t step = 0; step < 100; ++step) {
            nidx_t left = nidx_t(rng() % (n + 1));
            nidx_t right = left + nidx_t(rng() % (n - left + 1));
            if (rng() & 1) {
                char tag = char('a' + rng() % 5);
                tree.apply(left, right, tag);
                for (nidx_t i = left; i < right; ++i) values[i] = tag;
            } else {
                string expected;
                for (nidx_t i = left; i < right; ++i) expected += values[i];
                CHECK(tree.fold(left, right) == expected);
            }
        }
    }

    struct move_merge {
        unique_ptr<nidx_t> calls = make_unique<nidx_t>();
        string id() const { return {}; }
        string operator()(string left, const string& right) { ++*calls; return left += right; }
    };
    vector<string> tiny{"a", "b", "c"};
    nseg<string, move_merge> move_tree(nall(tiny), move_merge{});
    CHECK(move_tree.fold(0, 3) == "abc");
}
