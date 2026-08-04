namespace ni {
template <class T> constexpr nwide_t<T> ngeom_widen(const T& value) {
    using W = nwide_t<T>;
    if constexpr (is_arithmetic_v<T>)
        return nchecked_number<W>(value);
    else
        return W(value);
}
} // namespace ni

template <class T> struct npoint {
    T x{}, y{};

    constexpr npoint() = default;
    constexpr npoint(T x, T y) : x(move(x)), y(move(y)) {}
    template <class U> explicit constexpr operator npoint<U>() const {
        return {U(x), U(y)};
    }

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
    template <class U> constexpr npoint& operator*=(const U& scale) {
        x *= scale;
        y *= scale;
        return *this;
    }
    template <class U> constexpr npoint& operator/=(const U& scale) {
        x /= scale;
        y /= scale;
        return *this;
    }
    constexpr npoint operator+() const { return *this; }
    constexpr npoint operator-() const { return {-x, -y}; }
    friend constexpr npoint operator+(npoint a, const npoint& b) { return a += b; }
    friend constexpr npoint operator-(npoint a, const npoint& b) { return a -= b; }
    template <class U> friend constexpr npoint operator*(npoint a, const U& scale) {
        return a *= scale;
    }
    template <class U> friend constexpr npoint operator*(const U& scale, npoint a) {
        return a *= scale;
    }
    template <class U> friend constexpr npoint operator/(npoint a, const U& scale) {
        return a /= scale;
    }
    friend constexpr auto operator<=>(const npoint&, const npoint&) = default;
};

template <class T> constexpr nwide_t<T> ndot(const npoint<T>& a, const npoint<T>& b) {
    return ni::nchecked_add(ni::nchecked_mul(ni::ngeom_widen(a.x), ni::ngeom_widen(b.x)),
                            ni::nchecked_mul(ni::ngeom_widen(a.y), ni::ngeom_widen(b.y)));
}

template <class T> constexpr nwide_t<T> ncross(const npoint<T>& a, const npoint<T>& b) {
    return ni::nchecked_sub(ni::nchecked_mul(ni::ngeom_widen(a.x), ni::ngeom_widen(b.y)),
                            ni::nchecked_mul(ni::ngeom_widen(a.y), ni::ngeom_widen(b.x)));
}

template <class T>
constexpr nwide_t<T> ncross(const npoint<T>& a, const npoint<T>& b, const npoint<T>& c) {
    using W = nwide_t<T>;
    W abx = ni::nchecked_sub(ni::ngeom_widen(b.x), ni::ngeom_widen(a.x));
    W aby = ni::nchecked_sub(ni::ngeom_widen(b.y), ni::ngeom_widen(a.y));
    W acx = ni::nchecked_sub(ni::ngeom_widen(c.x), ni::ngeom_widen(a.x));
    W acy = ni::nchecked_sub(ni::ngeom_widen(c.y), ni::ngeom_widen(a.y));
    return ni::nchecked_sub(ni::nchecked_mul(abx, acy), ni::nchecked_mul(aby, acx));
}

template <class T> constexpr nwide_t<T> norient(const npoint<T>& a, const npoint<T>& b, const npoint<T>& c) {
    using W = nwide_t<T>;
    W abx = ni::nchecked_sub(ni::ngeom_widen(b.x), ni::ngeom_widen(a.x));
    W aby = ni::nchecked_sub(ni::ngeom_widen(b.y), ni::ngeom_widen(a.y));
    W acx = ni::nchecked_sub(ni::ngeom_widen(c.x), ni::ngeom_widen(a.x));
    W acy = ni::nchecked_sub(ni::ngeom_widen(c.y), ni::ngeom_widen(a.y));
    return ni::nchecked_sub(ni::nchecked_mul(abx, acy), ni::nchecked_mul(aby, acx));
}

template <class T> constexpr nwide_t<T> ndist2(const npoint<T>& a, const npoint<T>& b) {
    using W = nwide_t<T>;
    W dx = ni::nchecked_sub(ni::ngeom_widen(a.x), ni::ngeom_widen(b.x));
    W dy = ni::nchecked_sub(ni::ngeom_widen(a.y), ni::ngeom_widen(b.y));
    return ni::nchecked_add(ni::nchecked_mul(dx, dx), ni::nchecked_mul(dy, dy));
}

template <class X> constexpr int nsgn_eps(X value, long double epsilon = 0) {
    if constexpr (unsigned_integral<X>)
        return value > 0 ? 1 : 0;
    else if constexpr (signed_integral<X>)
        return value > 0 ? 1 : value < 0 ? -1 : 0;
    else
        return value > epsilon ? 1 : value < -epsilon ? -1 : 0;
}

template <class X> constexpr int nsign(X value) { return nsgn_eps(value); }

template <class T>
constexpr int norient(const npoint<T>& a, const npoint<T>& b, const npoint<T>& c,
                      long double epsilon) {
    return nsgn_eps(norient(a, b, c), epsilon);
}

template <class T> constexpr bool non_segment(const npoint<T>& point, const npoint<T>& a, const npoint<T>& b) {
    if (norient(a, b, point) != 0)
        return false;
    return min(a.x, b.x) <= point.x && point.x <= max(a.x, b.x) &&
           min(a.y, b.y) <= point.y && point.y <= max(a.y, b.y);
}

template <class T>
constexpr bool nonseg(const npoint<T>& a, const npoint<T>& b, const npoint<T>& point,
                      long double epsilon = 0) {
    if (norient(a, b, point, epsilon))
        return false;
    if constexpr (integral<T>) {
        return min(a.x, b.x) <= point.x && point.x <= max(a.x, b.x) &&
               min(a.y, b.y) <= point.y && point.y <= max(a.y, b.y);
    } else {
        return min(a.x, b.x) - epsilon <= point.x && point.x <= max(a.x, b.x) + epsilon &&
               min(a.y, b.y) - epsilon <= point.y && point.y <= max(a.y, b.y) + epsilon;
    }
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

template <class T>
constexpr bool nsegment_intersect(const npoint<T>& a, const npoint<T>& b, const npoint<T>& c,
                                  const npoint<T>& d, long double epsilon) {
    int ab_c = norient(a, b, c, epsilon), ab_d = norient(a, b, d, epsilon);
    int cd_a = norient(c, d, a, epsilon), cd_b = norient(c, d, b, epsilon);
    return (ab_c && ab_d && cd_a && cd_b)
               ? ab_c != ab_d && cd_a != cd_b
               : (!ab_c && nonseg(a, b, c, epsilon)) ||
                     (!ab_d && nonseg(a, b, d, epsilon)) ||
                     (!cd_a && nonseg(c, d, a, epsilon)) ||
                     (!cd_b && nonseg(c, d, b, epsilon));
}

template <class T> struct nline2 {
    npoint<T> p, v;
    template <class U> auto operator()(const U& scale) const { return p + v * scale; }
};

template <class T>
nmaybe<npoint<long double>> nline_intersect(const nline2<T>& a, const nline2<T>& b,
                                             long double epsilon = 0) {
    npoint<long double> av{static_cast<long double>(a.v.x), static_cast<long double>(a.v.y)};
    npoint<long double> bv{static_cast<long double>(b.v.x), static_cast<long double>(b.v.y)};
    npoint<long double> delta{static_cast<long double>(b.p.x) - static_cast<long double>(a.p.x),
                              static_cast<long double>(b.p.y) - static_cast<long double>(a.p.y)};
    long double denominator = ncross(av, bv);
    if (abs(denominator) <= epsilon)
        return {};
    long double scale = ncross(delta, bv) / denominator;
    return npoint<long double>{static_cast<long double>(a.p.x) + av.x * scale,
                               static_cast<long double>(a.p.y) + av.y * scale};
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
    npre(points.len() <= INT_MAX / 2);
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
        W term = ni::nchecked_sub(
            ni::nchecked_mul(ni::ngeom_widen(polygon[i].x), ni::ngeom_widen(polygon[next].y)),
            ni::nchecked_mul(ni::ngeom_widen(polygon[i].y), ni::ngeom_widen(polygon[next].x)));
        area = ni::nchecked_add(area, term);
    }
    return area;
}

template <nindexed A>
int npoint_in_poly(const A& polygon, const nindex_value_t<const A>& point,
                   long double epsilon = 0) {
    bool inside = false;
    for (int index = 0; index < nlen(polygon); ++index) {
        const auto& a = polygon[index];
        const auto& b = polygon[index + 1 == nlen(polygon) ? 0 : index + 1];
        if (nonseg(a, b, point, epsilon))
            return 0;
        int orientation = norient(a, b, point, epsilon);
        if ((a.y <= point.y && point.y < b.y && orientation > 0) ||
            (b.y <= point.y && point.y < a.y && orientation < 0))
            inside = !inside;
    }
    return inside ? 1 : -1;
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
