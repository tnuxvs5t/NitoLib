#include "../include/lazy_segtree.hpp"
#include <iostream>

struct AffineSum {
    using value_type = long long;
    struct tag_type { long long a = 1, b = 0; };
    long long id() const { return 0; }
    long long join(long long x, long long y) const { return x + y; }
    long long apply(long long sum, tag_type f, int length) const {
        return f.a * sum + f.b * length;
    }
    tag_type compose(tag_type newer, tag_type older) const {
        return {newer.a * older.a, newer.a * older.b + newer.b};
    }
};

int main() {
    std::vector<long long> values{1, 2, 3, 4};
    lazy_segtree<AffineSum> seg(values);
    seg.apply(0, 4, {2, 1}); // 3,5,7,9
    seg.apply(1, 3, {3, 4}); // 3,19,25,9
    std::cout << seg.all() << '\n';
    seg.set(2, 7);
    std::cout << seg.fold(1, 3) << '\n';
}
