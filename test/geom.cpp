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
    ntest(npoint_in_poly(hull, point{1, 1}) == 1);
    ntest(npoint_in_poly(hull, point{2, 1}) == 0);
    ntest(npoint_in_poly(hull, point{3, 1}) == -1);

    nvector<point> line{{0, 0}, {1, 0}, {2, 0}, {3, 0}};
    ntest(nconvex_hull(line).len() == 2);
    ntest(nconvex_hull(line, true) == line);

    auto intersection = nline_intersection(point{0, 0}, point{2, 2}, point{0, 2}, point{2, 0});
    ntest(intersection && abs(intersection->x - 1) < 1e-18L && abs(intersection->y - 1) < 1e-18L);
    auto line_intersection = nline_intersect(nline2<long long>{{0, 0}, {1, 1}},
                                              nline2<long long>{{0, 2}, {1, -1}});
    ntest(line_intersection && abs(line_intersection->x - 1) < 1e-18L &&
          abs(line_intersection->y - 1) < 1e-18L);
    using real_point = npoint<long double>;
    ntest(nonseg(real_point{0, 0}, real_point{2, 0}, real_point{1, 1e-12L}, 1e-9L));
    ntest(nsign(-3) == -1 && nsgn_eps(1e-12L, 1e-9L) == 0);
}
