#include "../src-v3/opt.hpp"

void check(bool condition, const char* message) {
    if (!condition) throw runtime_error(message);
}

struct line {
    long long slope, intercept;
    line() = delete;
    line(long long a, long long b) : slope(a), intercept(b) {}
};

struct eval {
    long long operator()(const line& item, int x) const { return item.slope * x + item.intercept; }
};

struct entry { line value; int left, right; };

int main() {
    constexpr long long infinity = (1LL << 60);
    mt19937 random(0x1c4a02026ULL);
    nlichao<line, int, long long, eval> minimum(-64, 65, infinity);
    vector<int> roots(8, -1);
    vector<vector<entry>> oracle(8);

    for (int step = 0; step < 30000; ++step) {
        int version = int(random() % roots.size());
        if (random() % 3) {
            line item(int(random() % 81) - 40, int(random() % 401) - 200);
            int left = -64, right = 65;
            if (random() & 1U) {
                left = int(random() % 129) - 64;
                right = left + 1 + int(random() % (65 - left));
                roots[version] = minimum.add_segment(roots[version], left, right, item);
            } else {
                roots[version] = minimum.add(roots[version], item);
            }
            oracle[version].push_back({item, left, right});
        } else {
            int x = int(random() % 129) - 64;
            long long expected = infinity;
            for (const auto& item : oracle[version]) if (item.left <= x && x < item.right)
                expected = min(expected, eval{}(item.value, x));
            check(minimum.query(roots[version], x) == expected, "minimum query");
        }
    }
    check(minimum.query(-1, 0) == infinity, "empty root");

    nlichao<nline<long long>, int, long long, nline_eval, greater<>> maximum(0, 20, -infinity);
    int root = -1;
    root = maximum.add(root, {2, 1});
    root = maximum.add(root, {-1, 40});
    for (int x = 0; x < 20; ++x)
        check(maximum.query(root, x) == max(2LL * x + 1, 40LL - x), "maximum query");
}
