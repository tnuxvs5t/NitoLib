#include "../src-v4/bag.hpp"
#include "../src-v4/vec_bag.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct pair_first_order {
    bool operator()(const pair<nidx_t, nidx_t>& left, nidx_t right) const {
        return left.first < right;
    }
    bool operator()(nidx_t left, const pair<nidx_t, nidx_t>& right) const {
        return left < right.first;
    }
};

struct item { nidx_t key, id; };
struct item_order {
    bool operator()(const item& left, const item& right) const {
        return left.key < right.key;
    }
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

template <class Bag>
void check_values(const Bag& bag, const vector<nidx_t>& expected) {
    CHECK(bag.len() == nidx_t(expected.size()));
    for (nidx_t i = 0; i < nidx_t(expected.size()); ++i) CHECK(bag[i] == expected[i]);
    auto view = bag.sequence();
    for (nidx_t i = 0; i < view.len(); ++i) CHECK(view[i] == expected[i]);
    if (!expected.empty()) CHECK(bag.front() == expected.front() && bag.back() == expected.back());
}

int main() {
    vector<nidx_t> source{4, 1, 4, -2, 7};
    nbag<nidx_t> tree_bag(nall(source));
    nvec_bag<nidx_t> vec_bag(nall(source));
    vector<nidx_t> reference{-2, 1, 4, 4, 7};
    check_values(tree_bag, reference);
    check_values(vec_bag, reference);

    vector<pair<nidx_t, nidx_t>> records{{2, 4}, {1, 8}, {2, 1}, {3, 0}};
    nbag<pair<nidx_t, nidx_t>> projected(nall(records));
    nvec_bag<pair<nidx_t, nidx_t>> projected_vec(nall(records));
    auto first = &pair<nidx_t, nidx_t>::first;
    CHECK(projected.lower_bound(2, pair_first_order{}) == 1);
    CHECK(projected.upper_bound(2, less<>{}, first) == 3);
    CHECK(projected_vec.lower_bound(2, pair_first_order{}) == 1);
    CHECK(projected_vec.upper_bound(2, less<>{}, first) == 3);

    vector<bool> bits{true, false, true, false};
    nbag<bool> bit_tree(nall(bits));
    nvec_bag<bool> bit_vec(nall(bits));
    CHECK(bit_tree.count(false) == 2 && bit_vec.count(true) == 2);

    vector<item> items{{2, 0}, {1, 1}, {2, 2}, {2, 3}, {1, 4}};
    nvec_bag<item, item_order> stable(nall(items));
    CHECK(stable[0].id == 1 && stable[1].id == 4 && stable[2].id == 0);
    CHECK(stable.emplace(2, 5) == 5 && stable[5].id == 5);

    nvec_bag<move_only> movable;
    movable.emplace(7);
    movable.emplace(3);
    CHECK(movable.erase_at(0).value == 3 && movable[0].value == 7);

    nbag<nidx_t> handles;
    nidx_t handle = handles.emplace(6);
    CHECK(handles.erase_handle(handle) && !handles.erase_handle(handle));

    mt19937 rng(0xBA65EED);
    for (nidx_t round = 0; round < 14000; ++round) {
        nidx_t operation = nidx_t(rng() % 7);
        nidx_t value = nidx_t(rng() % 201) - 100;
        if (operation < 3) {
            nidx_t position = nidx_t(upper_bound(reference.begin(), reference.end(), value) -
                                    reference.begin());
            nidx_t handle = tree_bag.insert(value);
            nidx_t vec_position = vec_bag.insert(value);
            CHECK(handle >= 0 && vec_position == position);
            reference.insert(reference.begin() + position, value);
        } else if (operation == 3) {
            auto it = lower_bound(reference.begin(), reference.end(), value);
            bool present = it != reference.end() && *it == value;
            if (present) reference.erase(it);
            CHECK(tree_bag.erase_one(value) == present);
            CHECK(vec_bag.erase_one(value) == present);
        } else if (operation == 4) {
            auto left = lower_bound(reference.begin(), reference.end(), value);
            auto right = upper_bound(reference.begin(), reference.end(), value);
            nidx_t count = nidx_t(right - left);
            reference.erase(left, right);
            CHECK(tree_bag.erase_all(value) == count);
            CHECK(vec_bag.erase_all(value) == count);
        } else if (operation == 5 && !reference.empty()) {
            nidx_t position = nidx_t(rng() % reference.size());
            nidx_t expected = reference[position];
            reference.erase(reference.begin() + position);
            CHECK(tree_bag.erase_at(position) == expected);
            CHECK(vec_bag.erase_at(position) == expected);
        } else {
            auto it = lower_bound(reference.begin(), reference.end(), value);
            bool present = it != reference.end() && *it == value;
            CHECK(tree_bag.contains(value) == present && vec_bag.contains(value) == present);
        }
        if (round % 97 == 0) {
            check_values(tree_bag, reference);
            check_values(vec_bag, reference);
        }
    }
    check_values(tree_bag, reference);
    check_values(vec_bag, reference);

    tree_bag.clear();
    vec_bag.clear();
    CHECK(tree_bag.empty() && vec_bag.empty());
    cout << "v4 bags: FHQ and vector ordered multisets passed\n";
}
