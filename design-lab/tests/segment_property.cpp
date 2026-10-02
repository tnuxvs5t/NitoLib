#include "../include/lazy_segtree.hpp"
#include "../include/segtree.hpp"
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <random>
#include <string>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x << '\n'; std::abort(); } } while (false)
struct Sum {
    using value_type = long long;
    long long id() const { return 0; }
    long long join(long long a, long long b) const { return a + b; }
};
struct Affine : Sum {
    struct tag_type { long long a = 1, b = 0; };
    long long apply(long long sum, tag_type f, int n) const { return f.a * sum + f.b * n; }
    tag_type compose(tag_type newer, tag_type older) const {
        return {newer.a * older.a, newer.a * older.b + newer.b};
    }
};
struct AssignString {
    using value_type = std::string;
    using tag_type = char;
    std::string id() const { return {}; }
    std::string join(const std::string& a, const std::string& b) const { return a + b; }
    std::string apply(const std::string&, char c, int n) const { return std::string(n, c); }
    char compose(char newer, char) const { return newer; }
};

int main() {
    std::vector<long long> empty;
    segtree<Sum> zero(empty);
    lazy_segtree<Affine> lazy_zero(empty);
    lazy_zero.apply(0, 0, {4, 7});
    CHECK(zero.all() == 0 && lazy_zero.all() == 0 && lazy_zero.fold(0, 0) == 0);
    CHECK(zero.max_right(0, [](long long v) { return v == 0; }) == 0);
    CHECK(zero.min_left(0, [](long long v) { return v == 0; }) == 0);
    std::vector<std::string> fixed{"a", "b", "c", "d", "e"};
    segtree<AssignString> ordered(fixed);
    CHECK(ordered.max_right(1, [](const std::string& x) { return std::string("bcd").starts_with(x); }) == 4);
    CHECK(ordered.min_left(4, [](const std::string& x) { return std::string("bcd").ends_with(x); }) == 1);
    std::mt19937 rng(6412);
    for (int trial = 0; trial < 1500; ++trial) {
        int n = 1 + int(rng() % 75);
        std::vector<long long> values(n), positive(n);
        std::vector<std::string> letters(n);
        for (int i = 0; i < n; ++i) {
            values[i] = int(rng() % 31) - 15;
            positive[i] = rng() % 10;
            letters[i] = std::string(1, char('a' + rng() % 6));
        }
        lazy_segtree<Affine> lazy(values);
        segtree<Sum> segment(positive);
        lazy_segtree<AssignString> text(letters);
        for (int query = 0; query < 120; ++query) {
            int l = int(rng() % unsigned(n + 1)), r = int(rng() % unsigned(n + 1));
            if (l > r) std::swap(l, r);
            if (query % 3 == 0) {
                Affine::tag_type f{int(rng() % 3) - 1, int(rng() % 15) - 7};
                lazy.apply(l, r, f);
                char c = char('a' + rng() % 6);
                text.apply(l, r, c);
                for (int i = l; i < r; ++i) { values[i] = f.a * values[i] + f.b; letters[i] = std::string(1, c); }
            } else if (query % 3 == 1) {
                int p = int(rng() % unsigned(n));
                values[p] = int(rng() % 41) - 20; lazy.set(p, values[p]);
                positive[p] = rng() % 10; segment.set(p, positive[p]);
                letters[p] = "x"; text.set(p, "x");
            }
            CHECK(lazy.fold(l, r) == std::accumulate(values.begin() + l, values.begin() + r, 0LL));
            CHECK(lazy.all() == std::accumulate(values.begin(), values.end(), 0LL));
            CHECK(text.fold(l, r) == std::accumulate(letters.begin() + l, letters.begin() + r, std::string{}));
            CHECK(segment.fold(l, r) == std::accumulate(positive.begin() + l, positive.begin() + r, 0LL));
            long long cap = rng() % 100, sum = 0;
            int right = l, left = r;
            while (right < n && sum + positive[right] <= cap) sum += positive[right++];
            CHECK(segment.max_right(l, [&](long long v) { return v <= cap; }) == right);
            sum = 0;
            while (left > 0 && positive[left - 1] + sum <= cap) sum += positive[--left];
            CHECK(segment.min_left(r, [&](long long v) { return v <= cap; }) == left);
        }
    }
    std::cout << "segment: 180000 random operations; affine composition, string order, boundaries, padding/empty\n";
}
