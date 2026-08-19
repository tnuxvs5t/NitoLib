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
    long long operator()(const line& item, nidx_t x) const { return item.slope * x + item.intercept; }
};

struct entry { line value; nidx_t left, right; };

int main() {
    constexpr long long infinity = (1LL << 60);
    mt19937 random(0x1c4a02026ULL);
    nlichao<line, nidx_t, long long, eval> minimum(-64, 65, infinity);
    vector<nidx_t> roots(8, -1);
    vector<vector<entry>> oracle(8);

    for (nidx_t step = 0; step < 30000; ++step) {
        nidx_t version = nidx_t(random() % roots.size());
        if (random() % 3) {
            line item(nidx_t(random() % 81) - 40, nidx_t(random() % 401) - 200);
            nidx_t left = -64, right = 65;
            if (random() & 1U) {
                left = nidx_t(random() % 129) - 64;
                right = left + 1 + nidx_t(random() % (65 - left));
                roots[version] = minimum.add_segment(roots[version], left, right, item);
            } else {
                roots[version] = minimum.add(roots[version], item);
            }
            oracle[version].push_back({item, left, right});
        } else {
            nidx_t x = nidx_t(random() % 129) - 64;
            long long expected = infinity;
            for (const auto& item : oracle[version]) if (item.left <= x && x < item.right)
                expected = min(expected, eval{}(item.value, x));
            check(minimum.query(roots[version], x) == expected, "minimum query");
        }
    }
    check(minimum.query(-1, 0) == infinity, "empty root");

    nlichao<nline<long long>, nidx_t, long long, nline_eval, greater<>> maximum(0, 20, -infinity);
    nidx_t root = -1;
    root = maximum.add(root, {2, 1});
    root = maximum.add(root, {-1, 40});
    for (nidx_t x = 0; x < 20; ++x)
        check(maximum.query(root, x) == max(2LL * x + 1, 40LL - x), "maximum query");
}
