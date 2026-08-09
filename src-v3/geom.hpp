#pragma once
#include "view.hpp"

template <class T>
struct npoint {
    T x{}, y{};
    constexpr npoint& operator+=(npoint other) { x += other.x, y += other.y; return *this; }
    constexpr npoint& operator-=(npoint other) { x -= other.x, y -= other.y; return *this; }
    constexpr npoint& operator*=(T scale) { x *= scale, y *= scale; return *this; }
    friend constexpr npoint operator+(npoint a, npoint b) { return a += b; }
    friend constexpr npoint operator-(npoint a, npoint b) { return a -= b; }
    friend constexpr npoint operator*(npoint a, T scale) { return a *= scale; }
    friend constexpr npoint operator*(T scale, npoint a) { return a *= scale; }
    friend constexpr auto operator<=>(const npoint&, const npoint&) = default;
};

template <class T>
constexpr auto ndot(npoint<T> a, npoint<T> b) { return a.x * b.x + a.y * b.y; }

template <class T>
constexpr auto ncross(npoint<T> a, npoint<T> b) { return a.x * b.y - a.y * b.x; }

template <class T>
constexpr auto ncross(npoint<T> a, npoint<T> b, npoint<T> c) { return ncross(b - a, c - a); }

template <class T>
constexpr bool non_segment(npoint<T> a, npoint<T> b, npoint<T> point) {
    return ncross(a, b, point) == 0 && min(a.x, b.x) <= point.x && point.x <= max(a.x, b.x) &&
           min(a.y, b.y) <= point.y && point.y <= max(a.y, b.y);
}

/* Exact inclusive intersection; coordinate products must stay representable. */
template <class T>
constexpr bool nsegment_intersect(npoint<T> a, npoint<T> b, npoint<T> c, npoint<T> d) {
    auto ab_c = ncross(a, b, c), ab_d = ncross(a, b, d);
    auto cd_a = ncross(c, d, a), cd_b = ncross(c, d, b);
    if (ab_c == 0 && non_segment(a, b, c)) return true;
    if (ab_d == 0 && non_segment(a, b, d)) return true;
    if (cd_a == 0 && non_segment(c, d, a)) return true;
    if (cd_b == 0 && non_segment(c, d, b)) return true;
    return (ab_c < 0) != (ab_d < 0) && (cd_a < 0) != (cd_b < 0);
}

template <class V>
auto npolygon_area2(V polygon) {
    using R = decltype(ncross(polygon[0], polygon[0]));
    R area{};
    for (int i = 0; i < polygon.len(); ++i)
        area += ncross(polygon[i], polygon[(i + 1) % polygon.len()]);
    return area;
}

/* Returns unique hull vertices counterclockwise, excluding collinear edge interiors. */
template <class V>
auto nconvex_hull(V points) {
    using P = remove_cvref_t<decltype(points[0])>;
    vector<P> sorted;
    sorted.reserve(points.len());
    for (int i = 0; i < points.len(); ++i) sorted.push_back(points[i]);
    sort(sorted.begin(), sorted.end());
    sorted.erase(unique(sorted.begin(), sorted.end()), sorted.end());
    if (sorted.size() <= 1) return sorted;
    vector<P> hull(2 * sorted.size());
    int size = 0;
    for (const P& point : sorted) {
        while (size >= 2 && ncross(hull[size - 2], hull[size - 1], point) <= 0) --size;
        hull[size++] = point;
    }
    int lower = size;
    for (int i = int(sorted.size()) - 2; i >= 0; --i) {
        const P& point = sorted[i];
        while (size > lower && ncross(hull[size - 2], hull[size - 1], point) <= 0) --size;
        hull[size++] = point;
    }
    hull.resize(size - 1);
    return hull;
}

/* Infinite lines; nullopt means parallel or coincident. */
template <class T>
optional<npoint<long double>>
nline_intersection(npoint<T> a, npoint<T> b, npoint<T> c, npoint<T> d) {
    auto first = b - a, second = d - c;
    auto denominator = ncross(first, second);
    if (denominator == 0) return nullopt;
    long double ratio = static_cast<long double>(ncross(c - a, second)) / denominator;
    return npoint<long double>{static_cast<long double>(a.x) + ratio * first.x,
                               static_cast<long double>(a.y) + ratio * first.y};
}
