template <class T> struct npoint {
    T x{}, y{};

    constexpr npoint& operator+=(const npoint& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    constexpr npoint& operator-=(const npoint& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    constexpr npoint& operator*=(const T& scale) {
        x *= scale;
        y *= scale;
        return *this;
    }
    friend constexpr npoint operator+(npoint a, const npoint& b) { return a += b; }
    friend constexpr npoint operator-(npoint a, const npoint& b) { return a -= b; }
    friend constexpr npoint operator*(npoint a, const T& scale) { return a *= scale; }
    friend constexpr bool operator==(const npoint&, const npoint&) = default;
};

template <class T> constexpr nwide_t<T> ndot(const npoint<T>& a, const npoint<T>& b) {
    using W = nwide_t<T>;
    return W(a.x) * W(b.x) + W(a.y) * W(b.y);
}

template <class T> constexpr nwide_t<T> ncross(const npoint<T>& a, const npoint<T>& b) {
    using W = nwide_t<T>;
    return W(a.x) * W(b.y) - W(a.y) * W(b.x);
}

template <class T> constexpr nwide_t<T> norient(const npoint<T>& a, const npoint<T>& b, const npoint<T>& c) {
    using W = nwide_t<T>;
    W abx = W(b.x) - W(a.x), aby = W(b.y) - W(a.y);
    W acx = W(c.x) - W(a.x), acy = W(c.y) - W(a.y);
    return abx * acy - aby * acx;
}

template <class T> constexpr nwide_t<T> ndist2(const npoint<T>& a, const npoint<T>& b) {
    using W = nwide_t<T>;
    W dx = W(a.x) - W(b.x), dy = W(a.y) - W(b.y);
    return dx * dx + dy * dy;
}

template <class T> constexpr bool non_segment(const npoint<T>& point, const npoint<T>& a, const npoint<T>& b) {
    if (norient(a, b, point) != 0)
        return false;
    return min(a.x, b.x) <= point.x && point.x <= max(a.x, b.x) &&
           min(a.y, b.y) <= point.y && point.y <= max(a.y, b.y);
}

template <class T>
constexpr bool nsegment_intersect(const npoint<T>& a, const npoint<T>& b, const npoint<T>& c,
                                  const npoint<T>& d) {
    auto ab_c = norient(a, b, c), ab_d = norient(a, b, d);
    auto cd_a = norient(c, d, a), cd_b = norient(c, d, b);
    if (ab_c == 0 && non_segment(c, a, b))
        return true;
    if (ab_d == 0 && non_segment(d, a, b))
        return true;
    if (cd_a == 0 && non_segment(a, c, d))
        return true;
    if (cd_b == 0 && non_segment(b, c, d))
        return true;
    return (ab_c < 0) != (ab_d < 0) && (cd_a < 0) != (cd_b < 0);
}

template <nindexed A> auto nconvex_hull(const A& source, bool keep_collinear = false) {
    using P = nindex_value_t<const A>;
    nvector<P> points;
    points.reserve(nlen(source));
    for (int i = 0; i < nlen(source); ++i)
        points.push(source[i]);
    nsort(points, [](const P& a, const P& b) { return a.x != b.x ? a.x < b.x : a.y < b.y; });
    int unique = 0;
    for (int i = 0; i < points.len(); ++i)
        if (!i || !(points[i] == points[i - 1]))
            points[unique++] = points[i];
    points.resize(unique);
    if (points.len() <= 2)
        return points;

    bool collinear = true;
    for (int i = 2; i < points.len(); ++i)
        collinear = collinear && norient(points[0], points[1], points[i]) == 0;
    if (collinear)
        return keep_collinear ? points : nvector<P>{points.front(), points.back()};

    nvector<P> hull;
    hull.reserve(2 * points.len());
    for (int i = 0; i < points.len(); ++i) {
        while (hull.len() >= 2) {
            auto turn = norient(hull[hull.len() - 2], hull.back(), points[i]);
            if (keep_collinear ? turn < 0 : turn <= 0)
                hull.pop();
            else
                break;
        }
        hull.push(points[i]);
    }
    int lower = hull.len();
    for (int i = points.len() - 2; i >= 0; --i) {
        while (hull.len() > lower) {
            auto turn = norient(hull[hull.len() - 2], hull.back(), points[i]);
            if (keep_collinear ? turn < 0 : turn <= 0)
                hull.pop();
            else
                break;
        }
        hull.push(points[i]);
    }
    hull.pop();
    return hull;
}

template <nindexed A> auto npolygon_area2(const A& polygon) {
    using P = nindex_value_t<const A>;
    using W = nwide_t<remove_cvref_t<decltype(declval<P>().x)>>;
    W area = 0;
    for (int i = 0; i < nlen(polygon); ++i) {
        int next = i + 1 == nlen(polygon) ? 0 : i + 1;
        area += W(polygon[i].x) * W(polygon[next].y) - W(polygon[i].y) * W(polygon[next].x);
    }
    return area;
}

template <nindexed A> auto nconvex_diameter2(const A& source) {
    auto hull = nconvex_hull(source);
    using P = nindex_value_t<decltype(hull)>;
    using W = decltype(ndist2(declval<P>(), declval<P>()));
    int n = hull.len();
    if (n <= 1)
        return W{};
    if (n == 2)
        return ndist2(hull[0], hull[1]);
    W answer = 0;
    int opposite = 1;
    for (int i = 0; i < n; ++i) {
        int next = i + 1 == n ? 0 : i + 1;
        while (norient(hull[i], hull[next], hull[(opposite + 1) % n]) >
               norient(hull[i], hull[next], hull[opposite]))
            opposite = (opposite + 1) % n;
        nchmax(answer, ndist2(hull[i], hull[opposite]));
        nchmax(answer, ndist2(hull[next], hull[opposite]));
    }
    return answer;
}

template <class T>
nmaybe<npoint<long double>> nline_intersection(const npoint<T>& a, const npoint<T>& b,
                                                const npoint<T>& c, const npoint<T>& d) {
    long double abx = (long double)b.x - (long double)a.x;
    long double aby = (long double)b.y - (long double)a.y;
    long double cdx = (long double)d.x - (long double)c.x;
    long double cdy = (long double)d.y - (long double)c.y;
    long double denominator = abx * cdy - aby * cdx;
    if (denominator == 0)
        return {};
    long double acx = (long double)c.x - (long double)a.x;
    long double acy = (long double)c.y - (long double)a.y;
    long double time = (acx * cdy - acy * cdx) / denominator;
    return npoint<long double>{(long double)a.x + abx * time, (long double)a.y + aby * time};
}
