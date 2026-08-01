#include "common.hpp"

int main() {
    mt19937 rng(34641016);
    using point = npoint<long long>;
    for (int trial = 0; trial < 3000; ++trial) {
        int n = int(rng() % 40);
        nvector<point> points;
        for (int i = 0; i < n; ++i)
            points.push(point{int(rng() % 41) - 20, int(rng() % 41) - 20});
        auto hull = nconvex_hull(points);
        if (hull.len() >= 3) {
            for (int i = 0; i < hull.len(); ++i)
                ntest(norient(hull[i], hull[(i + 1) % hull.len()], hull[(i + 2) % hull.len()]) > 0);
            for (int edge = 0; edge < hull.len(); ++edge)
                for (int i = 0; i < points.len(); ++i)
                    ntest(norient(hull[edge], hull[(edge + 1) % hull.len()], points[i]) >= 0);
        }

        __int128 expected = 0;
        for (int i = 0; i < points.len(); ++i)
            for (int j = i + 1; j < points.len(); ++j)
                expected = max(expected, ndist2(points[i], points[j]));
        ntest(nconvex_diameter2(points) == expected);
    }
}
