#include "common.hpp"

template <class S> int check_set(const S& actual, const set<int>& expected) {
    ntest(actual.len() == int(expected.size()));
    nvector<int> values;
    nfor(value, actual)
        values.push(value);
    ntest(values.len() == int(expected.size()));
    int position = 0;
    for (int value : expected)
        ntest(values[position++] == value);
    for (int value = -3; value <= 43; ++value) {
        ntest(actual.has(value) == expected.contains(value));
        ntest(actual.rank(value) == int(distance(expected.begin(), expected.lower_bound(value))));
        auto lower = actual.lower(value), upper = actual.upper(value);
        auto expected_lower = expected.lower_bound(value), expected_upper = expected.upper_bound(value);
        ntest(bool(lower) == (expected_lower != expected.end()));
        ntest(bool(upper) == (expected_upper != expected.end()));
        if (lower)
            ntest(lower.val() == *expected_lower);
        if (upper)
            ntest(upper.val() == *expected_upper);
    }
    for (int index = -1; index <= actual.len(); ++index) {
        auto value = actual.kth(index);
        ntest(bool(value) == (0 <= index && index < actual.len()));
        if (value) {
            auto expected_value = expected.begin();
            advance(expected_value, index);
            ntest(value.val() == *expected_value);
        }
    }
    return 0;
}

template <class S> int differential_set(uint32_t seed) {
    mt19937 random(seed);
    S actual;
    set<int> expected;
    for (int step = 0; step < 3000; ++step) {
        int value = int(random() % 41);
        if (random() & 1)
            ntest(bool(actual.ins(value)) == expected.insert(value).second);
        else
            ntest(bool(actual.del(value)) == bool(expected.erase(value)));
        if (step % 37 == 0)
            ntest(check_set(actual, expected) == 0);
    }
    return check_set(actual, expected);
}

struct zero_hash {
    size_t operator()(int) const { return 0; }
};

int main() {
    nseed(0x2301U);

    nheap<int> heap;
    priority_queue<int, vector<int>, greater<int>> reference_heap;
    mt19937 random(0x2302U);
    for (int step = 0; step < 5000; ++step) {
        if (reference_heap.empty() || random() % 3) {
            int value = int(random() % 1000);
            heap.push(value);
            reference_heap.push(value);
        } else {
            ntest(heap.top() == reference_heap.top());
            ntest(heap.pop() == reference_heap.top());
            reference_heap.pop();
        }
    }
    while (!reference_heap.empty()) {
        ntest(heap.pop() == reference_heap.top());
        reference_heap.pop();
    }
    ntest(heap.pop(17) == 17);

    ntest(differential_set<nset_fhq<int>>(0x2303U) == 0);
    ntest(differential_set<nset_splay<int>>(0x2304U) == 0);
    ntest(differential_set<nset_stl<int>>(0x2305U) == 0);

    nbag<int> bag;
    multiset<int> reference_bag;
    for (int step = 0; step < 4000; ++step) {
        int value = int(random() % 31), count = 1 + int(random() % 4);
        if (random() & 1) {
            ntest(bag.ins(value, count) == count);
            for (int repeat = 0; repeat < count; ++repeat)
                reference_bag.insert(value);
        } else {
            int removed = 0;
            for (; removed < count; ++removed) {
                auto found = reference_bag.find(value);
                if (found == reference_bag.end())
                    break;
                reference_bag.erase(found);
            }
            ntest(bag.del(value, count) == removed);
        }
        ntest(bag.len() == int(reference_bag.size()));
        if (step % 43 == 0) {
            int index = 0;
            for (int value_expected : reference_bag)
                ntest(bag.kth(index++).val() == value_expected);
        }
    }

    nset<int> a{1, 2, 4}, b{2, 3, 4};
    ntest((a | b) == nset<int>({1, 2, 3, 4}));
    ntest((a & b) == nset<int>({2, 4}));
    ntest((a - b) == nset<int>({1}));
    ntest((a ^ b) == nset<int>({1, 3}));

    nmap_flat<int, int, zero_hash> flat;
    nmap_hash<int, int, zero_hash> hashed;
    unordered_map<int, int> reference;
    for (int step = 0; step < 20000; ++step) {
        int key = int(random() % 500), operation = int(random() % 4);
        if (operation == 0) {
            int value = int(random());
            bool inserted = reference.emplace(key, value).second;
            ntest(flat.ins(key, value) == inserted);
            ntest(hashed.ins(key, value) == inserted);
        } else if (operation == 1) {
            int value = int(random());
            reference[key] = value;
            flat.set(key, value);
            hashed.set(key, value);
        } else if (operation == 2) {
            int removed = int(reference.erase(key));
            ntest(flat.del(key) == removed);
            ntest(hashed.del(key) == removed);
        } else {
            auto expected = reference.find(key);
            ntest(bool(flat.get(key)) == (expected != reference.end()));
            ntest(bool(hashed.get(key)) == (expected != reference.end()));
            if (expected != reference.end()) {
                ntest(*flat.get(key) == expected->second);
                ntest(*hashed.get(key) == expected->second);
            }
        }
        ntest(flat.len() == int(reference.size()));
        ntest(hashed.len() == int(reference.size()));
    }
    int visited = 0;
    nforkv(key, value, flat) {
        ntest(reference.at(key) == value);
        ++visited;
    }
    ntest(visited == flat.len());

    nrel<string, int> relation;
    ntest(relation.add("a", 1) && relation.add("a", 2) && !relation.add("a", 1));
    ntest(relation.image("a") == nvector<int>({1, 2}));
    ntest(relation.preimage(2) == nvector<string>({"a"}));

    npartial<string, int> partial;
    ntest(partial.bind("x", 4) && !partial.bind("x", 5));
    partial.set("x", 7);
    ntest(partial("x") == 7 && partial.unbind("x") && !partial.has("x"));

    nbije<string, int> bijection;
    ntest(bijection.bind("a", 2) && bijection.bind("b", 5));
    ntest(!bijection.bind("a", 5) && *bijection.to("a") == 2 && *bijection.from(5) == "b");
    auto inverse = ~bijection;
    ntest(*inverse.to(2) == "a" && *inverse.from("b") == 5);
    bijection.set("a", 7);
    ntest(!bijection.hasr(2) && *bijection.to("a") == 7);

    nvector<int> raw{30, 10, 30, 20};
    auto rank = ncompress(raw);
    ntest(rank.len() == 3 && rank.to(10) == 0 && rank.to(30) == 2 && rank.to(9) == npos);
    ntest(*rank.from(1) == 20 && *(~rank).to(2) == 30);
}
