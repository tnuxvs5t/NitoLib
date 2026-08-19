#include "../src-v3/ds.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

using aggregate_deque = ndeque_agg<string, concat>;
using aggregate_view = decltype(nall(declval<aggregate_deque&>()));
static_assert(same_as<decltype(declval<aggregate_deque&>()[0]), const string&>);
static_assert(same_as<decltype(declval<const aggregate_deque&>()[0]), const string&>);
static_assert(same_as<decltype(declval<const aggregate_view&>()[0]), const string&>);
static_assert(ranges::random_access_range<aggregate_view>);
static_assert(!indirectly_writable<ranges::iterator_t<aggregate_view>, string>);

void check(aggregate_deque& actual, const deque<string>& expected) {
    CHECK(actual.len() == nidx_t(expected.size()));
    CHECK(actual.empty() == expected.empty());
    string aggregate;
    for (const string& value : expected) aggregate += value;
    CHECK(actual.fold() == aggregate);
    auto view = nall(actual);
    CHECK(view.len() == nidx_t(expected.size()));
    for (nidx_t i = 0; i < view.len(); ++i)
        CHECK(view[i] == expected[size_t(i)] && actual[i] == expected[size_t(i)]);
    if (!expected.empty()) {
        CHECK(actual.front() == expected.front());
        CHECK(actual.back() == expected.back());
    }
}

int main() {
    aggregate_deque fixed;
    deque<string> fixed_reference;
    CHECK(fixed.fold().empty());
    for (char value = 'a'; value <= 'g'; ++value) {
        fixed.push_back(string(1, value));
        fixed_reference.push_back(string(1, value));
    }
    CHECK(fixed.front() == "a");
    for (nidx_t i = 0; i < 4; ++i) {
        fixed.pop_front();
        fixed_reference.pop_front();
    }
    CHECK(fixed.back() == "g");
    for (nidx_t i = 0; i < 2; ++i) {
        fixed.pop_back();
        fixed_reference.pop_back();
    }
    fixed.push_front("x");
    fixed_reference.push_front("x");
    fixed.push_back("y");
    fixed_reference.push_back("y");
    check(fixed, fixed_reference);

    aggregate_deque left_only;
    deque<string> left_reference;
    for (char value = 'a'; value <= 'g'; ++value) {
        left_only.push_front(string(1, value));
        left_reference.push_front(string(1, value));
    }
    CHECK(left_only.back() == "a");
    for (nidx_t i = 0; i < 4; ++i) {
        left_only.pop_back();
        left_reference.pop_back();
    }
    CHECK(left_only.front() == "g");
    check(left_only, left_reference);

    aggregate_deque alternating;
    deque<string> alternating_reference;
    for (nidx_t i = 0; i < 1000; ++i) {
        string value(1, char('a' + i % 6));
        alternating.push_back(value);
        alternating_reference.push_back(value);
    }
    for (nidx_t i = 0; i < 1000; ++i) {
        if (i & 1) {
            CHECK(alternating.back() == alternating_reference.back());
            alternating.pop_back();
            alternating_reference.pop_back();
        } else {
            CHECK(alternating.front() == alternating_reference.front());
            alternating.pop_front();
            alternating_reference.pop_front();
        }
    }
    check(alternating, alternating_reference);

    mt19937 rng(0xD3E9);
    for (nidx_t round = 0; round < 3000; ++round) {
        aggregate_deque actual;
        deque<string> expected;
        for (nidx_t step = 0; step < 300; ++step) {
            nidx_t operation = expected.empty() ? nidx_t(rng() & 1) : nidx_t(rng() % 6);
            if (operation == 0) {
                string value(1, char('a' + rng() % 6));
                actual.push_front(value);
                expected.push_front(value);
            } else if (operation == 1) {
                string value(1, char('a' + rng() % 6));
                actual.push_back(value);
                expected.push_back(value);
            } else if (operation == 2) {
                CHECK(actual.front() == expected.front());
                actual.pop_front();
                expected.pop_front();
            } else if (operation == 3) {
                CHECK(actual.back() == expected.back());
                actual.pop_back();
                expected.pop_back();
            } else if (operation == 4) {
                CHECK(actual.front() == expected.front());
            } else {
                CHECK(actual.back() == expected.back());
            }
            check(actual, expected);
        }
    }
}
