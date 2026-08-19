#include "../src-v3/segment.hpp"

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
    mt19937 rng(0x5E6);
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
