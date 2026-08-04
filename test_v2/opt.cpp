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

    nvector<long long> calls;
    ntest(nunimodal_arg(0LL, 10LL, [&](long long x) {
              calls.push(x);
              return (x - 4) * (x - 4);
          }) == 4);
    ntest(calls.len() >= 2 && calls[0] == 3 && calls[1] == 6);

    nvector<long long> coordinates{-10, -3, 0, 4, 4, 11};
    nlichao_static<long long> compressed(coordinates);
    ntest(compressed.len() == 5 && compressed.hasx(4) && !compressed.hasx(5));
    compressed.add(nline<long long>{2, 3});
    compressed.add(nline<long long>{-1, 7});
    ntest(compressed.get(-10).val() == -17);
    ntest(compressed.get(11).val() == -4);
    ntest(!compressed.get(5) && compressed(5, __int128_t(123)) == 123);

    nlichao_static<long long> segmented(coordinates);
    segmented.addseg(nline<long long>{1, 0}, -3, 11);
    ntest(!segmented.get(-10));
    ntest(segmented.get(-3).val() == -3 && segmented.get(4).val() == 4);
    ntest(!segmented.get(11));

    auto continuous = [](long double x) { return (x - 2.5L) * (x - 2.5L); };
    ntest(abs(nternary_min(-10.0L, 10.0L, continuous) - 2.5L) < 1e-9L);
}
