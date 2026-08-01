#include "common.hpp"

int main() {
    nlichao<long long> minimum(-100, 101);
    ntest(!minimum.query(0));
    minimum.add(2, 3);
    minimum.add(-1, 7);
    ntest(minimum.query(-10).val() == -17);
    ntest(minimum.query(10).val() == -3);

    mt19937 random(0x71c4a0U);
    struct segment_line {
        long long slope, intercept, left, right;
    };
    for (int repeat = 0; repeat < 220; ++repeat) {
        nlichao<long long> tree(-50, 51);
        nvector<segment_line> lines;
        int count = 1 + random() % 80;
        for (int i = 0; i < count; ++i) {
            long long slope = int(random() % 41) - 20;
            long long intercept = int(random() % 101) - 50;
            long long left = int(random() % 101) - 50;
            long long right = left + 1 + random() % (51 - left);
            lines.push(slope, intercept, left, right);
            tree.add_segment(slope, intercept, left, right);
        }
        for (long long x = -50; x <= 50; ++x) {
            nmaybe<__int128_t> brute;
            for (int i = 0; i < lines.len(); ++i)
                if (lines[i].left <= x && x < lines[i].right) {
                    __int128_t value = __int128_t(lines[i].slope) * x + lines[i].intercept;
                    if (!brute || value < brute.val())
                        brute = value;
                }
            auto got = tree.query(x);
            ntest(bool(got) == bool(brute));
            if (got)
                ntest(got.val() == brute.val());
        }
    }

    nlichao<long long, ngreater<__int128_t>> maximum(-20, 21);
    maximum.add(3, -2);
    maximum.add(-4, 5);
    for (long long x = -20; x <= 20; ++x)
        ntest(maximum.query(x).val() == max(3 * x - 2, -4 * x + 5));

    auto bowl = [](long long x) { return (x - 37) * (x - 37) + 9; };
    ntest(nunimodal_arg(-100LL, 101LL, bowl) == 37);
    auto hill = [](long long x) { return 1000 - (x + 11) * (x + 11); };
    ntest(nunimodal_arg(-100LL, 101LL, hill, ngreater<>{}) == -11);
}
