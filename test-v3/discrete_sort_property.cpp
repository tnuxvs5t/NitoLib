#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct nsum_action {
    long long value = 0;
    void operator()(int x) { value += x; }
};

struct nmove_compare {
    unique_ptr<int> bias = make_unique<int>(0);
    nmove_compare() = default;
    nmove_compare(nmove_compare&&) = default;
    nmove_compare(const nmove_compare&) = delete;
    bool operator()(int left, int right) const { return left + *bias > right + *bias; }
};

int main() {
    vector<int> values{8, 3, 7, 1, 9, 2};
    CHECK((nprefix(nall(values)) == vector<int>{0, 8, 11, 18, 19, 28, 30}));
    CHECK((nsuffix(nall(values)) == vector<int>{30, 22, 19, 12, 11, 2, 0}));
    vector<string> letters{"a", "b", "c"};
    auto join = [](const string& left, const string& right) {
        return "(" + left + "+" + right + ")";
    };
    CHECK((nprefix(nall(letters), string("I"), join) ==
           vector<string>{"I", "(I+a)", "((I+a)+b)", "(((I+a)+b)+c)"}));
    CHECK((nsuffix(nall(letters), string("I"), join) ==
           vector<string>{"(a+(b+(c+I)))", "(b+(c+I))", "(c+I)", "I"}));
    CHECK((ncollect(nstride(nall(values), 0, 6, 2)) == vector<int>{8, 7, 9}));
    CHECK((ncollect(nstride(nall(values), -2)) == vector<int>{2, 1, 3}));
    auto invertible_stride = nstride(nrange(10, 30), 1, 19, 3);
    for (int i = 0; i < invertible_stride.len(); ++i)
        CHECK(invertible_stride.inverse(invertible_stride[i]) == i);
    auto negative_stride = nstride(nrange(6), -2);
    for (int i = 0; i < negative_stride.len(); ++i)
        CHECK(negative_stride.inverse(negative_stride[i]) == i);

    CHECK(naccumulate(nall(values), 0) == 30);
    CHECK(naccumulate(nall(values), string{}, [](string accumulated, int value) {
        return accumulated + char('0' + value);
    }) == "837192");
    CHECK(neach(nall(values), nsum_action{}).value == 30);
    CHECK(nfind_if(nall(values), [](int value) { return value < 2; }) == 3);
    CHECK(nfind_if(nall(values), [](int value) { return value < 0; }) == 6);
    CHECK(ncount_if(nall(values), [](int value) { return value & 1; }) == 4);
    CHECK(nall_of(nall(values), [](int value) { return value > 0; }));
    CHECK(nany_of(nall(values), [](int value) { return value == 7; }));
    CHECK(nnone_of(nall(values), [](int value) { return value == 6; }));
    CHECK(nargmin(nall(values)) == 3 && nargmax(nall(values)) == 4);
    vector<int> empty;
    CHECK(nargmin(nall(empty)) == 0 && nargmax(nall(empty)) == 0);
    CHECK((nprefix(nall(empty)) == vector<int>{0}));
    CHECK((nsuffix(nall(empty), 7) == vector<int>{7}));

    vector<pair<int, int>> records{{4, 0}, {1, 1}, {3, 2}, {2, 3}};
    CHECK(nargmin(nall(records), less<>{}, &pair<int, int>::first) == 1);
    auto record_order = norder(nall(records), greater<>{}, &pair<int, int>::first);
    CHECK(record_order[0].first == 4 && record_order[3].first == 1);
    CHECK(records[0] == pair(4, 0));
    nsort(nall(records), less<>{}, &pair<int, int>::first);
    CHECK((records == vector<pair<int, int>>{{1, 1}, {2, 3}, {3, 2}, {4, 0}}));
    vector<int> move_sorted{2, 5, 1, 4, 3};
    nsort(nall(move_sorted), nmove_compare{});
    CHECK((move_sorted == vector<int>{5, 4, 3, 2, 1}));
    auto move_ordered = norder(nall(move_sorted), nmove_compare{});
    CHECK((ncollect(move_ordered) == move_sorted));

    vector<int> sorted{1, 2, 2, 2, 5, 9};
    CHECK(nlower(nall(sorted), 0) == 0);
    CHECK(nlower(nall(sorted), 2) == 1 && nupper(nall(sorted), 2) == 4);
    CHECK(nlower(nall(sorted), 10) == 6 && nupper(nall(sorted), 10) == 6);

    vector<int> keys{40, 10, 30, 20};
    vector<int> payload{4, 1, 3, 2};
    array<int, 41> locate{};
    for (int i = 0; i < 4; ++i) locate[keys[i]] = i;
    auto function = nfunc_bind(nall(keys), nall(payload),
                               [&](int key) { return locate[key]; });
    nsort(function, greater<>{});
    CHECK((keys == vector<int>{40, 10, 30, 20}));
    CHECK((payload == vector<int>{4, 3, 2, 1}));
    for (int i = 0; i < function.len(); ++i) CHECK(function[i] == function(function.key(i)));
    nreverse_inplace(function);
    CHECK((payload == vector<int>{1, 2, 3, 4}));

    mt19937 rng(0x5077);
    for (int round = 0; round < 12000; ++round) {
        int n = int(rng() % 50);
        vector<int> input(n);
        for (int& value : input) value = int(rng() % 101) - 50;

        vector<int> expected = input;
        ranges::sort(expected);
        vector<int> actual = input;
        nsort(nall(actual));
        CHECK(actual == expected);

        vector<int> untouched = input;
        auto ordered = norder(nall(untouched));
        CHECK(ncollect(ordered) == expected && untouched == input);
        auto plan = nargsort(nall(input));
        CHECK(int(plan.size()) == n);
        vector<int> seen(n);
        for (int i = 0; i < n; ++i) {
            CHECK(0 <= plan[i] && plan[i] < n);
            ++seen[plan[i]];
            if (i) CHECK(input[plan[i - 1]] <= input[plan[i]]);
        }
        for (int count : seen) CHECK(count == 1);

        vector<int> reversed = input;
        vector<int> descending = input;
        ranges::sort(descending, greater<>{});
        nsort(nreverse(nall(reversed)));
        CHECK(reversed == descending);

        vector<int> strided = input, brute = input;
        vector<int> positions;
        for (int i = 0; i < n; i += 2) positions.push_back(i);
        vector<int> picked;
        for (int position : positions) picked.push_back(brute[position]);
        ranges::sort(picked);
        for (int i = 0; i < int(positions.size()); ++i) brute[positions[i]] = picked[i];
        nsort(nstride(nall(strided), 2));
        CHECK(strided == brute);
    }
}
