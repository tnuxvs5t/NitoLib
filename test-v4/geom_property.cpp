#include "../src-v4/geom.hpp"
#include "../src-v4/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using point = npoint<long long>;

bool brute_intersect(point a, point b, point c, point d) {
    auto on = [](point left, point right, point value) {
        return ncross(right - left, value - left) == 0 &&
               min(left.x, right.x) <= value.x && value.x <= max(left.x, right.x) &&
               min(left.y, right.y) <= value.y && value.y <= max(left.y, right.y);
    };
    if (a == b) return on(c, d, a);
    if (c == d) return on(a, b, c);
    point r = b - a, s = d - c;
    long long denominator = ncross(r, s);
    long long first = ncross(c - a, s), second = ncross(c - a, r);
    if (denominator) {
        if (denominator < 0) denominator = -denominator, first = -first, second = -second;
        return 0 <= first && first <= denominator && 0 <= second && second <= denominator;
    }
    if (ncross(c - a, r)) return false;
    return max(min(a.x, b.x), min(c.x, d.x)) <= min(max(a.x, b.x), max(c.x, d.x)) &&
           max(min(a.y, b.y), min(c.y, d.y)) <= min(max(a.y, b.y), max(c.y, d.y));
}

int main() {
    mt19937 rng(0x6e0);
    for (nidx_t round = 0; round < 70000; ++round) {
        auto make_point = [&] { return point{nidx_t(rng() % 31) - 15, nidx_t(rng() % 31) - 15}; };
        point a = make_point(), b = make_point(), c = make_point(), d = make_point();
        CHECK(nsegment_intersect(a, b, c, d) == brute_intersect(a, b, c, d));
    }
    for (nidx_t round = 0; round < 8000; ++round) {
        nidx_t n = nidx_t(rng() % 40);
        vector<point> points(n);
        for (auto& value : points) value = {nidx_t(rng() % 31) - 15, nidx_t(rng() % 31) - 15};
        auto hull = nconvex_hull(nall(points));
        CHECK(ranges::adjacent_find(hull) == hull.end());
        if (hull.size() >= 3) {
            CHECK(npolygon_area2(nall(hull)) > 0);
            for (nidx_t i = 0; i < nidx_t(hull.size()); ++i) {
                point a = hull[i], b = hull[(i + 1) % hull.size()];
                CHECK(ncross(a, b, hull[(i + 2) % hull.size()]) > 0);
                for (point value : points) CHECK(ncross(a, b, value) >= 0);
            }
        } else if (hull.size() == 2) {
            for (point value : points) CHECK(ncross(hull[0], hull[1], value) == 0);
        }
    }
    vector<point> square{{0, 0}, {2, 0}, {2, 2}, {0, 2}, {1, 0}, {1, 1}, {0, 0}};
    vector<point> expected_hull{{0, 0}, {2, 0}, {2, 2}, {0, 2}};
    CHECK(nconvex_hull(nall(square)) == expected_hull);
    auto crossing = nline_intersection(point{0, 0}, point{2, 2}, point{0, 2}, point{2, 0});
    CHECK(crossing && abs(crossing->x - 1) < 1e-18L && abs(crossing->y - 1) < 1e-18L);
    CHECK(!nline_intersection(point{0, 0}, point{1, 0}, point{0, 1}, point{1, 1}));
    cout << "v4 geom: exact orientation, inclusive intersections, hull and line crossing passed\n";
}
