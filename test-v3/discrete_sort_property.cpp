#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct nsum_action {
    long long value = 0;
    void operator()(nidx_t x) { value += x; }
};

struct nmove_compare {
    unique_ptr<nidx_t> bias = make_unique<nidx_t>(0);
    nmove_compare() = default;
    nmove_compare(nmove_compare&&) = default;
    nmove_compare(const nmove_compare&) = delete;
    bool operator()(nidx_t left, nidx_t right) const { return left + *bias > right + *bias; }
};

int main() {
    vector<nidx_t> values{8, 3, 7, 1, 9, 2};
    CHECK((nprefix(nall(values)) == vector<nidx_t>{0, 8, 11, 18, 19, 28, 30}));
    CHECK((nsuffix(nall(values)) == vector<nidx_t>{30, 22, 19, 12, 11, 2, 0}));
    vector<string> letters{"a", "b", "c"};
    auto join = [](const string& left, const string& right) {
        return "(" + left + "+" + right + ")";
    };
    CHECK((nprefix(nall(letters), string("I"), join) ==
           vector<string>{"I", "(I+a)", "((I+a)+b)", "(((I+a)+b)+c)"}));
    CHECK((nsuffix(nall(letters), string("I"), join) ==
           vector<string>{"(a+(b+(c+I)))", "(b+(c+I))", "(c+I)", "I"}));
    CHECK((ncollect(nstride(nall(values), 0, 6, 2)) == vector<nidx_t>{8, 7, 9}));
    CHECK((ncollect(nstride(nall(values), -2)) == vector<nidx_t>{2, 1, 3}));
    auto invertible_stride = nstride(nrange(10, 30), 1, 19, 3);
    for (nidx_t i = 0; i < invertible_stride.len(); ++i)
        CHECK(invertible_stride.inverse(invertible_stride[i]) == i);
    auto negative_stride = nstride(nrange(6), -2);
    for (nidx_t i = 0; i < negative_stride.len(); ++i)
        CHECK(negative_stride.inverse(negative_stride[i]) == i);

    CHECK(naccumulate(nall(values), 0) == 30);
    CHECK(naccumulate(nall(values), string{}, [](string accumulated, nidx_t value) {
        return accumulated + char('0' + value);
    }) == "837192");
    CHECK(neach(nall(values), nsum_action{}).value == 30);
    CHECK(nfind_if(nall(values), [](nidx_t value) { return value < 2; }) == 3);
    CHECK(nfind_if(nall(values), [](nidx_t value) { return value < 0; }) == 6);
    CHECK(ncount_if(nall(values), [](nidx_t value) { return value & 1; }) == 4);
    CHECK(nall_of(nall(values), [](nidx_t value) { return value > 0; }));
    CHECK(nany_of(nall(values), [](nidx_t value) { return value == 7; }));
    CHECK(nnone_of(nall(values), [](nidx_t value) { return value == 6; }));
    CHECK(nargmin(nall(values)) == 3 && nargmax(nall(values)) == 4);
    vector<nidx_t> empty;
    CHECK(nargmin(nall(empty)) == 0 && nargmax(nall(empty)) == 0);
    CHECK((nprefix(nall(empty)) == vector<nidx_t>{0}));
    CHECK((nsuffix(nall(empty), nidx_t(7)) == vector<nidx_t>{7}));

    vector<pair<nidx_t, nidx_t>> records{{4, 0}, {1, 1}, {3, 2}, {2, 3}};
    CHECK(nargmin(nall(records), less<>{}, &pair<nidx_t, nidx_t>::first) == 1);
    auto record_order = norder(nall(records), greater<>{}, &pair<nidx_t, nidx_t>::first);
    CHECK(record_order[0].first == 4 && record_order[3].first == 1);
    CHECK(records[0] == pair(4, 0));
    nsort(nall(records), less<>{}, &pair<nidx_t, nidx_t>::first);
    CHECK((records == vector<pair<nidx_t, nidx_t>>{{1, 1}, {2, 3}, {3, 2}, {4, 0}}));
    vector<nidx_t> move_sorted{2, 5, 1, 4, 3};
    nsort(nall(move_sorted), nmove_compare{});
    CHECK((move_sorted == vector<nidx_t>{5, 4, 3, 2, 1}));
    auto move_ordered = norder(nall(move_sorted), nmove_compare{});
    CHECK((ncollect(move_ordered) == move_sorted));

    vector<nidx_t> sorted{1, 2, 2, 2, 5, 9};
    CHECK(nlower(nall(sorted), 0) == 0);
    CHECK(nlower(nall(sorted), 2) == 1 && nupper(nall(sorted), 2) == 4);
    CHECK(nlower(nall(sorted), 10) == 6 && nupper(nall(sorted), 10) == 6);

    vector<nidx_t> keys{40, 10, 30, 20};
    vector<nidx_t> payload{4, 1, 3, 2};
    array<nidx_t, 41> locate{};
    for (nidx_t i = 0; i < 4; ++i) locate[keys[i]] = i;
    auto function = nanchors(nall(keys), nall(payload),
                               [&](nidx_t key) { return locate[key]; });
    nsort(function, greater<>{});
    CHECK((keys == vector<nidx_t>{40, 10, 30, 20}));
    CHECK((payload == vector<nidx_t>{4, 3, 2, 1}));
    for (nidx_t i = 0; i < function.len(); ++i) CHECK(function[i] == function(function.key(i)));
    nreverse_inplace(function);
    CHECK((payload == vector<nidx_t>{1, 2, 3, 4}));

    mt19937 rng(0x5077);
    for (nidx_t round = 0; round < 12000; ++round) {
        nidx_t n = nidx_t(rng() % 50);
        vector<nidx_t> input(n);
        for (nidx_t& value : input) value = nidx_t(rng() % 101) - 50;

        vector<nidx_t> expected = input;
        ranges::sort(expected);
        vector<nidx_t> actual = input;
        nsort(nall(actual));
        CHECK(actual == expected);

        vector<nidx_t> untouched = input;
        auto ordered = norder(nall(untouched));
        CHECK(ncollect(ordered) == expected && untouched == input);
        auto plan = nargsort(nall(input));
        CHECK(nidx_t(plan.size()) == n);
        vector<nidx_t> seen(n);
        for (nidx_t i = 0; i < n; ++i) {
            CHECK(0 <= plan[i] && plan[i] < n);
            ++seen[plan[i]];
            if (i) CHECK(input[plan[i - 1]] <= input[plan[i]]);
        }
        for (nidx_t count : seen) CHECK(count == 1);

        vector<nidx_t> reversed = input;
        vector<nidx_t> descending = input;
        ranges::sort(descending, greater<>{});
        nsort(nreverse(nall(reversed)));
        CHECK(reversed == descending);

        vector<nidx_t> strided = input, brute = input;
        vector<nidx_t> positions;
        for (nidx_t i = 0; i < n; i += 2) positions.push_back(i);
        vector<nidx_t> picked;
        for (nidx_t position : positions) picked.push_back(brute[position]);
        ranges::sort(picked);
        for (nidx_t i = 0; i < nidx_t(positions.size()); ++i) brute[positions[i]] = picked[i];
        nsort(nstride(nall(strided), 2));
        CHECK(strided == brute);
    }
}
