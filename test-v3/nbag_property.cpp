#include "../src-v3/bag.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

template <class V>
concept nbag_ctad = requires(V source) { nbag(source); };

using integer_bag = nbag<nidx_t>;
static_assert(is_const_v<remove_reference_t<decltype((declval<const integer_bag&>().tree))>>);
static_assert(is_const_v<remove_reference_t<decltype((declval<const integer_bag&>().root))>>);

int main() {
    vector<nidx_t> initial{4, 1, 4, -2, 7};
    static_assert(!nbag_ctad<decltype(nall(initial))>);
    nbag<nidx_t> bag(nall(initial));
    multiset<nidx_t> reference(initial.begin(), initial.end());

    auto verify = [&] {
        CHECK(bag.len() == nidx_t(reference.size()));
        vector<nidx_t> actual;
        auto sequence = bag.sequence();
        for (nidx_t i = 0; i < sequence.len(); ++i) actual.push_back(sequence[i]);
        vector<nidx_t> expected(reference.begin(), reference.end());
        CHECK(actual == expected);
        for (nidx_t i = 0; i < bag.len(); ++i) CHECK(bag.kth(i) == expected[i]);
        if (!expected.empty()) CHECK(bag.front() == expected.front() && bag.back() == expected.back());
        for (nidx_t key = -10; key <= 10; ++key) {
            auto lower = reference.lower_bound(key), upper = reference.upper_bound(key);
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
    auto stable_sequence = bag.sequence();
    vector<array<nidx_t, 4>> stable_shape;
    for (nidx_t handle = 0; handle < bag.nodes(); ++handle) {
        const auto& node = bag.tree[handle];
        stable_shape.push_back({node.left, node.right, node.parent, node.size});
    }
    nidx_t stable_root = bag.root;
    for (nidx_t key = -10; key <= 10; ++key) {
        (void)bag.lower_bound(key);
        (void)bag.upper_bound(key);
        (void)bag.equal_range(key);
        (void)bag.count(key);
        (void)bag.find(key);
        (void)bag.contains(key);
    }
    CHECK(bag.root == stable_root);
    for (nidx_t handle = 0; handle < bag.nodes(); ++handle) {
        const auto& node = bag.tree[handle];
        CHECK((stable_shape[handle] == array<nidx_t, 4>{node.left, node.right,
                                                        node.parent, node.size}));
    }
    vector<nidx_t> stable_values;
    for (nidx_t i = 0; i < stable_sequence.len(); ++i)
        stable_values.push_back(stable_sequence[i]);
    CHECK(stable_values == vector<nidx_t>(reference.begin(), reference.end()));

    vector<nidx_t> first{3, 1, 2}, second{30, 10, 20};
    nbag<tuple<nidx_t, nidx_t>> zipped(nzip(nall(first), nall(second)));
    first = {300, 100, 200};
    second = {-3, -1, -2};
    vector<tuple<nidx_t, nidx_t>> zipped_values;
    for (nidx_t i = 0; i < zipped.len(); ++i) zipped_values.push_back(zipped[i]);
    CHECK((zipped_values == vector<tuple<nidx_t, nidx_t>>{{1, 10}, {2, 20}, {3, 30}}));

    vector<bool> bits{true, false, true, false};
    nbag<bool> bit_bag(nall(bits));
    bits.assign(bits.size(), false);
    CHECK(bit_bag.count(false) == 2 && bit_bag.count(true) == 2);

    nbag<nidx_t> handles;
    nidx_t handle = handles.emplace(7);
    CHECK(handles.erase_handle(handle) && !handles.erase_handle(handle) && handles.empty());

    mt19937 rng(0xBA65EED);
    nidx_t inserted_nodes = bag.nodes();
    for (nidx_t round = 0; round < 30000; ++round) {
        nidx_t operation = nidx_t(rng() % 7);
        nidx_t value = nidx_t(rng() % 201) - 100;
        if (operation < 3) {
            nidx_t inserted = bag.insert(value);
            ++inserted_nodes;
            reference.insert(value);
            if (round % 37 == 0) {
                CHECK(bag.erase_handle(inserted));
                reference.erase(reference.find(value));
            }
        } else if (operation == 3) {
            auto it = reference.find(value);
            bool expected = it != reference.end();
            if (expected) reference.erase(it);
            CHECK(bag.erase_one(value) == expected);
        } else if (operation == 4) {
            nidx_t expected = nidx_t(reference.count(value));
            reference.erase(value);
            CHECK(bag.erase_all(value) == expected);
        } else if (operation == 5 && !reference.empty()) {
            nidx_t position = nidx_t(rng() % reference.size());
            auto it = next(reference.begin(), position);
            nidx_t expected = *it;
            reference.erase(it);
            CHECK(bag.erase_at(position) == expected);
        } else {
            CHECK(bag.contains(value) == (reference.find(value) != reference.end()));
        }
        if (round % 127 == 0) verify();
    }
    verify();
    CHECK(bag.nodes() == inserted_nodes && bag.nodes() > 10000);

    bag.clear();
    reference.clear();
    CHECK(bag.empty() && bag.nodes() == 0);

    vector<nidx_t> descending_source{1, 5, 3, 5};
    nbag<nidx_t, greater<>> descending(nall(descending_source));
    vector<nidx_t> descending_values;
    auto descending_sequence = descending.sequence();
    for (nidx_t i = 0; i < descending_sequence.len(); ++i)
        descending_values.push_back(descending_sequence[i]);
    CHECK((descending_values == vector<nidx_t>{5, 5, 3, 1}));
    CHECK(descending.lower_bound(4) == 2 && descending.upper_bound(5) == 2);
}
