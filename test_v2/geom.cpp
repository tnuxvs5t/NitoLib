#include "common.hpp"

int main() {
    using point = npoint<long long>;
    point a{3000000000LL, 0}, b{0, 3000000000LL};
    ntest(ncross(a, b) == __int128_t(9000000000000000000LL));
    ntest(norient(point{-3000000000LL, 0}, point{3000000000LL, 0}, point{0, 3000000000LL}) > 0);

    ntest(nsegment_intersect(point{0, 0}, point{4, 4}, point{0, 4}, point{4, 0}));
    ntest(nsegment_intersect(point{0, 0}, point{4, 0}, point{2, 0}, point{7, 0}));
    ntest(!nsegment_intersect(point{0, 0}, point{1, 0}, point{2, 0}, point{3, 0}));

    nvector<point> points{{0, 0}, {2, 0}, {2, 2}, {0, 2}, {1, 1}, {1, 0}, {0, 0}};
    auto hull = nconvex_hull(points);
    ntest((hull == nvector<point>{{0, 0}, {2, 0}, {2, 2}, {0, 2}}));
    ntest(npolygon_area2(hull) == 8);
    ntest(nconvex_diameter2(points) == 8);

    nvector<point> line{{0, 0}, {1, 0}, {2, 0}, {3, 0}};
    ntest(nconvex_hull(line).len() == 2);
    ntest(nconvex_hull(line, true) == line);

    auto intersection = nline_intersection(point{0, 0}, point{2, 2}, point{0, 2}, point{2, 0});
    ntest(intersection && abs(intersection->x - 1) < 1e-18L && abs(intersection->y - 1) < 1e-18L);
}
