#include "../src-v3/vec_bag.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

template <class V>
concept nvec_bag_ctad = requires(V source) { nvec_bag(source); };

struct pair_first_order {
    bool operator()(const pair<nidx_t, nidx_t>& left, nidx_t right) const {
        return left.first < right;
    }

    bool operator()(nidx_t left, const pair<nidx_t, nidx_t>& right) const {
        return left < right.first;
    }
};

struct item {
    nidx_t key, id;
};

struct item_order {
    bool operator()(const item& left, const item& right) const { return left.key < right.key; }
};

struct noncopyable_less {
    noncopyable_less() = default;
    noncopyable_less(const noncopyable_less&) = delete;
    noncopyable_less& operator=(const noncopyable_less&) = delete;
    noncopyable_less(noncopyable_less&&) = default;
    noncopyable_less& operator=(noncopyable_less&&) = default;
    bool operator()(nidx_t left, nidx_t right) const { return left < right; }
};

struct move_only {
    nidx_t value;
    explicit move_only(nidx_t x) : value(x) {}
    move_only(const move_only&) = delete;
    move_only& operator=(const move_only&) = delete;
    move_only(move_only&&) = default;
    move_only& operator=(move_only&&) = default;
    friend bool operator<(const move_only& left, const move_only& right) {
        return left.value < right.value;
    }
};

int main() {
    vector<nidx_t> initial{4, 1, 4, -2, 7};
    static_assert(!nvec_bag_ctad<decltype(nall(initial))>);
    static_assert(same_as<decltype(declval<const nvec_bag<nidx_t>&>().kth(0)), const nidx_t&>);
    static_assert(same_as<decltype(declval<const nvec_bag<bool>&>().kth(0)), bool>);

    nvec_bag<nidx_t> bag(nall(initial));
    vector<nidx_t> reference(initial.begin(), initial.end());
    ranges::sort(reference);

    auto verify = [&] {
        CHECK(bag.len() == nidx_t(reference.size()));
        auto sequence = bag.sequence();
        for (nidx_t i = 0; i < sequence.len(); ++i) CHECK(sequence[i] == reference[i]);
        for (nidx_t i = 0; i < bag.len(); ++i) CHECK(bag.kth(i) == reference[i]);
        if (!reference.empty())
            CHECK(bag.front() == reference.front() && bag.back() == reference.back());
        for (nidx_t key = -10; key <= 10; ++key) {
            auto lower = lower_bound(reference.begin(), reference.end(), key);
            auto upper = upper_bound(reference.begin(), reference.end(), key);
            nidx_t left = nidx_t(distance(reference.begin(), lower));
            nidx_t right = nidx_t(distance(reference.begin(), upper));
            CHECK(bag.lower_bound(key) == left);
            CHECK(bag.upper_bound(key) == right);
            CHECK(bag.order_of_key(key) == left);
            CHECK(bag.equal_range(key) == pair(left, right));
            CHECK(bag.count(key) == right - left);
            CHECK(bag.find(key) == (left < right ? left : bag.len()));
            CHECK(bag.contains(key) == (left < right));
        }
    };

    verify();

    nvec_bag<nidx_t> copied = bag;
    nvec_bag<nidx_t> assigned;
    assigned = bag;
    copied.erase_one(-2);
    assigned.insert(9);
    CHECK(bag.len() == 5 && bag[0] == -2 && bag[1] == 1 && bag[2] == 4 &&
          bag[3] == 4 && bag[4] == 7);
    CHECK(copied.len() == 4 && copied[0] == 1 && copied[1] == 4 && copied[2] == 4 &&
          copied[3] == 7);
    CHECK(assigned.len() == 6 && assigned[0] == -2 && assigned[1] == 1 &&
          assigned[2] == 4 && assigned[3] == 4 && assigned[4] == 7 && assigned[5] == 9);

    auto stable_sequence = bag.sequence();
    for (nidx_t key = -10; key <= 10; ++key) {
        (void)bag.lower_bound(key);
        (void)bag.upper_bound(key);
        (void)bag.equal_range(key);
        (void)bag.count(key);
        (void)bag.find(key);
        (void)bag.contains(key);
    }
    for (nidx_t i = 0; i < stable_sequence.len(); ++i) CHECK(stable_sequence[i] == reference[i]);

    vector<nidx_t> first{3, 1, 2}, second{30, 10, 20};
    nvec_bag<tuple<nidx_t, nidx_t>> zipped(nzip(nall(first), nall(second)));
    first = {300, 100, 200};
    second = {-3, -1, -2};
    vector<tuple<nidx_t, nidx_t>> zipped_values;
    for (nidx_t i = 0; i < zipped.len(); ++i) zipped_values.push_back(zipped[i]);
    CHECK((zipped_values == vector<tuple<nidx_t, nidx_t>>{{1, 10}, {2, 20}, {3, 30}}));

    vector<bool> bits{true, false, true, false};
    nvec_bag<bool> bit_bag(nall(bits));
    CHECK(bit_bag.kth(0) == false && bit_bag.kth(1) == false);
    CHECK(bit_bag.kth(2) == true && bit_bag.kth(3) == true);
    CHECK(bit_bag.front() == false && bit_bag.back() == true);
    bits.assign(bits.size(), true);
    CHECK(bit_bag.count(false) == 2 && bit_bag.count(true) == 2);

    vector<pair<nidx_t, nidx_t>> records{{2, 4}, {1, 8}, {2, 1}, {3, 0}};
    nvec_bag<pair<nidx_t, nidx_t>> projected(nall(records));
    auto first_member = &pair<nidx_t, nidx_t>::first;
    CHECK(projected.lower_bound(2, pair_first_order{}) == 1);
    CHECK(projected.upper_bound(2, pair_first_order{}) == 3);
    CHECK(projected.lower_bound(2, less<>{}, first_member) == 1);
    CHECK(projected.upper_bound(2, less<>{}, first_member) == 3);

    vector<item> items{{2, 0}, {1, 1}, {2, 2}, {2, 3}, {1, 4}};
    nvec_bag<item, item_order> stable(nall(items));
    vector<nidx_t> expected_ids{1, 4, 0, 2, 3};
    for (nidx_t i = 0; i < stable.len(); ++i) CHECK(stable[i].id == expected_ids[i]);
    CHECK(stable.emplace(2, 5) == 5);
    CHECK(stable[5].id == 5);

    nvec_bag<nidx_t, noncopyable_less> custom(noncopyable_less{});
    custom.insert(3);
    custom.insert(1);
    CHECK(custom.lower_bound(2) == 1 && custom.upper_bound(3) == 2);

    nvec_bag<move_only> movable;
    movable.emplace(7);
    movable.emplace(3);
    auto removed = movable.erase_at(0);
    CHECK(removed.value == 3 && movable[0].value == 7);
    movable.clear();
    CHECK(movable.empty());

    mt19937 rng(0xBA65EED);
    for (nidx_t round = 0; round < 12000; ++round) {
        nidx_t operation = nidx_t(rng() % 7);
        nidx_t value = nidx_t(rng() % 201) - 100;
        if (operation < 3) {
            nidx_t expected = nidx_t(upper_bound(reference.begin(), reference.end(), value) -
                                     reference.begin());
            CHECK(bag.insert(value) == expected);
            reference.insert(reference.begin() + expected, value);
        } else if (operation == 3) {
            auto it = lower_bound(reference.begin(), reference.end(), value);
            bool expected = it != reference.end() && *it == value;
            if (expected) reference.erase(it);
            CHECK(bag.erase_one(value) == expected);
        } else if (operation == 4) {
            auto left_it = lower_bound(reference.begin(), reference.end(), value);
            auto right_it = upper_bound(reference.begin(), reference.end(), value);
            nidx_t expected = nidx_t(right_it - left_it);
            reference.erase(left_it, right_it);
            CHECK(bag.erase_all(value) == expected);
        } else if (operation == 5 && !reference.empty()) {
            nidx_t position = nidx_t(rng() % reference.size());
            nidx_t expected = reference[position];
            reference.erase(reference.begin() + position);
            CHECK(bag.erase_at(position) == expected);
        } else {
            auto it = lower_bound(reference.begin(), reference.end(), value);
            CHECK(bag.contains(value) == (it != reference.end() && *it == value));
        }
        if (round % 127 == 0) verify();
    }
    verify();

    bag.clear();
    reference.clear();
    CHECK(bag.empty() && bag.len() == 0);

    vector<nidx_t> descending_source{1, 5, 3, 5};
    nvec_bag<nidx_t, greater<>> descending(nall(descending_source));
    CHECK(descending[0] == 5 && descending[1] == 5 && descending[2] == 3 && descending[3] == 1);
    CHECK(descending.lower_bound(4) == 2 && descending.upper_bound(5) == 2);
}
