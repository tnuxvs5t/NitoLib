#pragma once
#include <bits/stdc++.h>
using namespace std;

// 00 nconfig / 01 nbase
#ifndef NDEBUG
#define nassert(x) assert(x)
#else
#define nassert(x) ((void)0)
#endif

inline constexpr int npos = -1;
template <class T>
inline constexpr T ninf = [] {
    if constexpr (numeric_limits<T>::has_infinity)
        return numeric_limits<T>::infinity();
    else
        return numeric_limits<T>::max() / 4;
}();
template <class T>
inline constexpr T nninf = [] {
    if constexpr (numeric_limits<T>::has_infinity)
        return -numeric_limits<T>::infinity();
    else if constexpr (is_signed_v<T>)
        return numeric_limits<T>::lowest() / 4;
    else
        return T{};
}();

template <class T> struct nmaybe {
    optional<T> x;
    constexpr nmaybe() = default;
    constexpr nmaybe(const T& v) : x(v) {}
    constexpr nmaybe(T&& v) : x(move(v)) {}
    constexpr bool ok() const { return x.has_value(); }
    constexpr explicit operator bool() const { return ok(); }
    constexpr T& val() {
        nassert(ok());
        return *x;
    }
    constexpr const T& val() const {
        nassert(ok());
        return *x;
    }
    constexpr T val(T d) const& { return x ? *x : move(d); }
    constexpr T val(T d) && { return x ? move(*x) : move(d); }
    constexpr T& operator*() { return val(); }
    constexpr const T& operator*() const { return val(); }
    constexpr T* operator->() { return &val(); }
    constexpr const T* operator->() const { return &val(); }
    constexpr void reset() { x.reset(); }
};

template <class T, class U> constexpr bool nchmin(T& a, U&& b) {
    if (b < a)
        return a = forward<U>(b), true;
    return false;
}
template <class T, class U> constexpr bool nchmax(T& a, U&& b) {
    if (a < b)
        return a = forward<U>(b), true;
    return false;
}
template <class T> constexpr int nsign(const T& x) {
    return T{} < x ? 1 : x < T{} ? -1 : 0;
}
template <integral T> constexpr make_unsigned_t<T> nmag(T x) {
    using U = make_unsigned_t<T>;
    U u = U(x);
    if constexpr (is_signed_v<T>)
        return x < 0 ? U(0) - u : u;
    else
        return u;
}
template <integral T> constexpr auto nabs(T x) {
    return nmag(x);
}
template <class T>
    requires(!integral<T>)
constexpr T nabs(T x) {
    return x < T{} ? -x : x;
}
template <class T> constexpr int ncmp(const T& a, const T& b) {
    return a < b ? -1 : b < a ? 1 : 0;
}
template <class A> constexpr int nlen(const A& a) {
    if constexpr (requires { a.len(); })
        return a.len();
    else
        return int(a.size());
}
template <class T>
using nwide_t =
    conditional_t<integral<T>,
                  conditional_t<(sizeof(T) < 8), conditional_t<is_signed_v<T>, long long, unsigned long long>,
                                conditional_t<is_signed_v<T>, __int128_t, __uint128_t>>,
                  long double>;
inline constexpr int nbitceil(int n) {
    nassert(n >= 0);
    if (n <= 1)
        return 1;
    if (n > (1 << 30)) {
        nassert(false);
        return 1 << 30;
    }
    return int(bit_ceil(unsigned(n)));
}

template <class T = void> struct nless {
    constexpr bool operator()(const auto& a, const auto& b) const { return a < b; }
};
template <class T = void> struct ngreater {
    constexpr bool operator()(const auto& a, const auto& b) const { return b < a; }
};
template <class T = void> struct nequal {
    constexpr bool operator()(const auto& a, const auto& b) const { return a == b; }
};

struct nrng {
    using result_type = uint64_t;
    uint64_t s;
    constexpr nrng(uint64_t x = 0x243f6a8885a308d3ULL) : s(x) {}
    static constexpr uint64_t min() { return 0; }
    static constexpr uint64_t max() { return ~0ULL; }
    static constexpr uint64_t mix(uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }
    constexpr uint64_t operator()() { return s = mix(s); }
    template <integral T> constexpr T operator()(T n) {
        nassert(n > 0);
        return T((__uint128_t((*this)()) * make_unsigned_t<T>(n)) >> 64);
    }
    template <integral T> constexpr T operator()(T l, T r) {
        nassert(l < r);
        using U = make_unsigned_t<T>;
        U d = U(r) - U(l), x = (*this)(d);
        using W = nwide_t<T>;
        return T(W(l) + W(x));
    }
};
inline uint64_t nseed_value =
    uint64_t(chrono::steady_clock::now().time_since_epoch().count()) ^ uint64_t(uintptr_t(&nseed_value));
inline nrng nrng_global(nseed_value);
inline uint64_t nhash_seed = nrng::mix(nseed_value);
inline void nseed(uint64_t s) {
    nseed_value = s;
    nrng_global = nrng(s);
    nhash_seed = nrng::mix(s);
}

template <class T> struct nhash {
    uint64_t s = nhash_seed;
    nhash() = default;
    explicit nhash(uint64_t s) : s(s) {}
    size_t operator()(const T& x) const noexcept { return size_t(nrng::mix(uint64_t(hash<T>{}(x)) + s)); }
};
template <class A, class B> struct nhash<pair<A, B>> {
    uint64_t s = nhash_seed;
    nhash() = default;
    explicit nhash(uint64_t s) : s(s) {}
    size_t operator()(const pair<A, B>& x) const noexcept {
        return size_t(nrng::mix(uint64_t(nhash<A>{s}(x.first)) ^
                                (uint64_t(nhash<B>{s ^ 0x9e3779b97f4a7c15ULL}(x.second)) + 0x9e3779b97f4a7c15ULL)));
    }
};

#define ncat0(a, b) a##b
#define ncat(a, b) ncat0(a, b)

namespace ni {
template <class T> struct hold {
    T x;
    constexpr hold(T&& y) : x(move(y)) {}
    constexpr T& get() { return x; }
    constexpr const T& get() const { return x; }
    constexpr T&& take() { return move(x); }
};
template <class T> struct hold<T&> {
    T* p;
    constexpr hold(T& x) : p(&x) {}
    constexpr T& get() const { return *p; }
    constexpr T& take() const { return *p; }
};
} // namespace ni

// 02 nenum
template <class T> struct nrange_cursor {
    T x, r, d;
    int k = 0;
    constexpr bool ok() const { return d && (d > 0 ? x < r : x > r); }
    constexpr T val() const { return x; }
    constexpr int idx() const { return k; }
    constexpr void next() {
        if (!d)
            return;
        if constexpr (is_signed_v<T>) {
            if ((d > 0 && x > numeric_limits<T>::max() - d) || (d < 0 && x < numeric_limits<T>::lowest() - d))
                x = r;
            else
                x += d;
        } else if (x > numeric_limits<T>::max() - d)
            x = r;
        else
            x += d;
        ++k;
    }
};
template <class T> struct nrange_t {
    T l, r, d;
    constexpr nrange_t() = default;
    constexpr nrange_t(T l, T r, T d = 1) : l(l), r(r), d(d) { nassert(d != T{}); }
    constexpr auto enumerate() const { return nrange_cursor<T>{l, r, d}; }
    constexpr int len() const {
        if (!d) {
            nassert(false);
            return 0;
        }
        if (d > 0) {
            if (l >= r)
                return 0;
            using W = nwide_t<T>;
            W z = (W(r) - W(l) + W(d) - 1) / W(d);
            nassert(z <= numeric_limits<int>::max());
            return int(z);
        }
        if (l <= r)
            return 0;
        using W = nwide_t<T>;
        W z = (W(l) - W(r) - W(d) - 1) / (-W(d));
        nassert(z <= numeric_limits<int>::max());
        return int(z);
    }
};
template <integral T> constexpr auto nrange(T r) {
    return nrange_t<T>{0, r, 1};
}
template <integral A, integral B> constexpr auto nrange(A l, B r) {
    using T = common_type_t<A, B>;
    return nrange_t<T>{T(l), T(r), 1};
}
template <integral A, integral B, integral C> constexpr auto nrange(A l, B r, C d) {
    using T = common_type_t<A, B, C>;
    nassert(d != 0);
    return nrange_t<T>{T(l), T(r), T(d)};
}
template <class A> constexpr decltype(auto) nenumerate(A&& a) {
    return forward<A>(a).enumerate();
}

#define nrep0(i, z, k) for (int i = 0, ncat(_nz, k) = int(z); i < ncat(_nz, k); ++i)
#define nrep1(i, z, k) nrep0(i, z, k)
#define nrep(i, ...) nrep1(i, (__VA_ARGS__), __COUNTER__)
#define nrrep0(i, z, k) for (int i = int(z) - 1; i >= 0; --i)
#define nrrep1(i, z, k) nrrep0(i, z, k)
#define nrrep(i, ...) nrrep1(i, (__VA_ARGS__), __COUNTER__)
#define nfor0(x, a, k)                                                                                                 \
    for (auto ncat(_ne, k) = nenumerate(a); ncat(_ne, k).ok(); ncat(_ne, k).next())                                    \
        if (auto&& x = ncat(_ne, k).val(); false) {                                                                    \
        } else
#define nfor1(x, a, k) nfor0(x, a, k)
#define nfor(x, ...) nfor1(x, (__VA_ARGS__), __COUNTER__)
#define nfori0(i, x, a, k)                                                                                             \
    for (auto ncat(_ne, k) = nenumerate(a); ncat(_ne, k).ok(); ncat(_ne, k).next())                                    \
        if (int i = ncat(_ne, k).idx(); false) {                                                                       \
        } else if (auto&& x = ncat(_ne, k).val(); false) {                                                             \
        } else
#define nfori1(i, x, a, k) nfori0(i, x, a, k)
#define nfori(i, x, ...) nfori1(i, x, (__VA_ARGS__), __COUNTER__)
#define nforkv0(k, v, a, z)                                                                                            \
    for (auto ncat(_ne, z) = nenumerate(a); ncat(_ne, z).ok(); ncat(_ne, z).next())                                    \
        if (auto&& k = ncat(_ne, z).key(); false) {                                                                    \
        } else if (auto&& v = ncat(_ne, z).val(); false) {                                                             \
        } else
#define nforkv1(k, v, a, z) nforkv0(k, v, a, z)
#define nforkv(k, v, ...) nforkv1(k, v, (__VA_ARGS__), __COUNTER__)

// 03 nmem
template <class T> struct nspan_cursor {
    T* p;
    int n, i = 0;
    constexpr bool ok() const { return i < n; }
    constexpr T& val() const { return p[i]; }
    constexpr int idx() const { return i; }
    constexpr void next() { ++i; }
};
template <class T> struct nspan {
    T* p = nullptr;
    int n = 0;
    constexpr nspan() = default;
    constexpr nspan(T* q, int m) : p(q), n(m) { nassert(m >= 0); }
    template <class A>
        requires requires(A& a) {
            a.data();
            a.size();
        }
    constexpr nspan(A& a) : p(a.data()), n(int(a.size())) {}
    constexpr int len() const { return n; }
    constexpr bool empty() const { return !n; }
    constexpr T& operator[](int i) const {
        nassert(0 <= i && i < n);
        return p[i];
    }
    constexpr T* get(int i) const { return 0 <= i && i < n ? p + i : nullptr; }
    constexpr remove_const_t<T> get(int i, remove_const_t<T> d) const { return 0 <= i && i < n ? p[i] : move(d); }
    constexpr nspan sub(int l, int r) const {
        nassert(0 <= l && l <= r && r <= n);
        return {p + l, r - l};
    }
    constexpr nspan sub(int l) const { return sub(l, n); }
    constexpr auto enumerate() const { return nspan_cursor<T>{p, n}; }
};

template <class T> struct npool_dynamic {
    vector<optional<T>> a;
    vector<int> fr;
    int live = 0;
    int len() const { return live; }
    int cap() const { return int(a.size()); }
    bool empty() const { return !live; }
    void reserve(int n) {
        nassert(n >= 0);
        a.reserve(max(0, n));
        fr.reserve(max(0, n));
    }
    template <class... A> int make(A&&... x) {
        int i;
        if (fr.empty())
            i = int(a.size()) + 1, a.emplace_back(in_place, forward<A>(x)...);
        else
            i = fr.back(), fr.pop_back(), a[i - 1].emplace(forward<A>(x)...);
        ++live;
        return i;
    }
    void del(int i) {
        bool ok = 0 < i && i <= int(a.size()) && a[i - 1].has_value();
        nassert(ok);
        if (!ok)
            return;
        a[i - 1].reset();
        fr.push_back(i);
        --live;
    }
    T& operator[](int i) {
        bool ok = 0 < i && i <= int(a.size()) && a[i - 1].has_value();
        nassert(ok);
        return *a[i - 1];
    }
    const T& operator[](int i) const {
        bool ok = 0 < i && i <= int(a.size()) && a[i - 1].has_value();
        nassert(ok);
        return *a[i - 1];
    }
    T* get(int i) { return 0 < i && i <= int(a.size()) && a[i - 1] ? &*a[i - 1] : nullptr; }
    const T* get(int i) const { return 0 < i && i <= int(a.size()) && a[i - 1] ? &*a[i - 1] : nullptr; }
    void clear() {
        a.clear();
        fr.clear();
        live = 0;
    }
};
template <class T> using npool = npool_dynamic<T>;

enum class nbranch : unsigned char { left, take, right };

// Read-only node views are topology snapshots. Any tree mutation may expire them.
template <class S> struct nnode {
    using value_type = typename S::value_type;
    using info_type = typename S::info_type;

  private:
    const S* s = nullptr;
    int u = 0;
    uint64_t epoch = 0;
    constexpr nnode(const S* owner, int handle, uint64_t version) : s(owner), u(handle), epoch(version) {}
    friend S;

  public:
    constexpr nnode() = default;
    bool current() const { return s && epoch == s->nnode_epoch(); }
    bool ok() const { return current() && u && s->nnode_alive(u); }
    explicit operator bool() const { return ok(); }
    const value_type& val() const {
        nassert(ok());
        return s->nnode_val(u);
    }
    int count() const {
        nassert(current());
        return s->nnode_count(u);
    }
    int len() const {
        nassert(current());
        return s->nnode_len(u);
    }
    info_type info() const {
        nassert(current());
        return s->nnode_info(u);
    }
    nnode left() const {
        nassert(current());
        return {s, s->nnode_left(u), epoch};
    }
    nnode right() const {
        nassert(current());
        return {s, s->nnode_right(u), epoch};
    }
    int handle() const { return u; }
};

template <class S>
concept nnode_tree = requires(const S& s) {
    { s.root() } -> same_as<nnode<S>>;
};

template <nnode_tree S, class F> nnode<S> nwalk(const S& s, F&& decide) {
    auto u = s.root();
    while (u) {
        auto d = invoke(decide, u);
        if (d == nbranch::left)
            u = u.left();
        else if (d == nbranch::right)
            u = u.right();
        else {
            nassert(d == nbranch::take);
            return u;
        }
    }
    return u;
}

// 04 nop
template <class T> struct nadd {
    static constexpr bool commutative = true;
    static constexpr T op(T a, const T& b) { return a += b; }
    static constexpr T id() { return T{}; }
    static constexpr T inv(T a) { return -a; }
};
template <class T> struct nmul {
    static constexpr T op(T a, const T& b) { return a *= b; }
    static constexpr T id() { return T{1}; }
};
template <class T> struct nxor {
    static constexpr bool commutative = true;
    static constexpr T op(T a, const T& b) { return a ^= b; }
    static constexpr T id() { return T{}; }
    static constexpr T inv(T a) { return a; }
};
template <class T> struct nmin {
    static constexpr bool commutative = true;
    static constexpr T op(const T& a, const T& b) { return b < a ? b : a; }
    static constexpr T id() { return ninf<T>; }
};
template <class T> struct nmax {
    static constexpr bool commutative = true;
    static constexpr T op(const T& a, const T& b) { return a < b ? b : a; }
    static constexpr T id() { return nninf<T>; }
};
template <class O, class T>
concept nsemigroup = requires(T a, T b) {
    { O::op(a, b) } -> convertible_to<T>;
};
template <class O, class T>
concept nmonoid = nsemigroup<O, T> && requires {
    { O::id() } -> convertible_to<T>;
};
template <class O, class T>
concept ncommutative_monoid = nmonoid<O, T> && requires {
    { O::commutative } -> convertible_to<bool>;
    requires bool(O::commutative);
};
template <class O, class T>
concept ngroup = nmonoid<O, T> && requires(T a) {
    { O::inv(a) } -> convertible_to<T>;
};

template <class A, class T>
concept naugment = copyable<typename A::info_type> && requires(const A& a, const T& x, int c, typename A::info_type p) {
    { a.id() } -> convertible_to<typename A::info_type>;
    { a.one(x, c) } -> convertible_to<typename A::info_type>;
    { a.op(p, p) } -> convertible_to<typename A::info_type>;
};

template <class T> struct nempty_augment {
    using info_type = monostate;
    constexpr info_type id() const { return {}; }
    constexpr info_type one(const T&, int) const { return {}; }
    constexpr info_type op(info_type, info_type) const { return {}; }
};

template <class S>
concept naugmented_tree = nnode_tree<S> && requires(const S& s) {
    typename S::augment_type;
    { s.augment() } -> same_as<const typename S::augment_type&>;
};

// Prefix/suffix search treats all equal values in one tree node as one augmented block.
template <naugmented_tree S, class P> nnode<S> nfirst_prefix(const S& s, P&& pred) {
    const auto& a = s.augment();
    auto u = s.root();
    auto acc = a.id();
    while (u) {
        auto l = u.left();
        auto left = a.op(acc, l.info());
        if (invoke(pred, left)) {
            u = l;
            continue;
        }
        auto through = a.op(left, a.one(u.val(), u.count()));
        if (invoke(pred, through))
            return u;
        acc = move(through);
        u = u.right();
    }
    return u;
}

template <naugmented_tree S, class P> nnode<S> nlast_suffix(const S& s, P&& pred) {
    const auto& a = s.augment();
    auto u = s.root();
    auto acc = a.id();
    while (u) {
        auto r = u.right();
        auto right = a.op(r.info(), acc);
        if (invoke(pred, right)) {
            u = r;
            continue;
        }
        auto through = a.op(a.one(u.val(), u.count()), right);
        if (invoke(pred, through))
            return u;
        acc = move(through);
        u = u.left();
    }
    return u;
}

template <class T, class O = nmul<T>>
    requires nmonoid<O, T>
constexpr T npow(T a, long long e, O = {}) {
    uint64_t k;
    if (e < 0) {
        if constexpr (ngroup<O, T>)
            a = O::inv(a);
        else {
            nassert(e >= 0);
            return O::id();
        }
        k = 0 - uint64_t(e);
    } else
        k = e;
    T r = O::id();
    for (; k; k >>= 1, a = O::op(a, a))
        if (k & 1)
            r = O::op(r, a);
    return r;
}

// 10 nseq / cursor compositions
template <class A> struct nindex_cursor {
    A* a;
    int i, r, d, k = 0;
    constexpr bool ok() const { return d > 0 ? i < r : i > r; }
    constexpr decltype(auto) val() const { return (*a)[i]; }
    constexpr int idx() const { return k; }
    constexpr void next() {
        i += d;
        ++k;
    }
};

template <class T> struct nvector_stl {
    vector<T> a;
    nvector_stl() = default;
    explicit nvector_stl(int n) : a(n >= 0 ? size_t(n) : 0) { nassert(n >= 0); }
    nvector_stl(int n, const T& x) : a(n >= 0 ? size_t(n) : 0, x) { nassert(n >= 0); }
    nvector_stl(initializer_list<T> x) : a(x) {}
    int len() const { return int(a.size()); }
    int cap() const { return int(a.capacity()); }
    bool empty() const { return a.empty(); }
    void reserve(int n) { a.reserve(n); }
    void resize(int n) { a.resize(n); }
    void resize(int n, const T& x) { a.resize(n, x); }
    void clear() { a.clear(); }
    T* data() { return a.data(); }
    const T* data() const { return a.data(); }
    T& operator[](int i) {
        nassert(0 <= i && i < len());
        return a[i];
    }
    const T& operator[](int i) const {
        nassert(0 <= i && i < len());
        return a[i];
    }
    T* get(int i) { return 0 <= i && i < len() ? &a[i] : nullptr; }
    const T* get(int i) const { return 0 <= i && i < len() ? &a[i] : nullptr; }
    T get(int i, T d) const { return 0 <= i && i < len() ? a[i] : move(d); }
    template <class... A> T& push(A&&... x) { return a.emplace_back(forward<A>(x)...); }
    T pop() {
        nassert(!empty());
        T x = move(a.back());
        a.pop_back();
        return x;
    }
    T pop(T d) { return empty() ? move(d) : pop(); }
    T& front() {
        nassert(!empty());
        return a.front();
    }
    const T& front() const {
        nassert(!empty());
        return a.front();
    }
    T front(T d) const { return empty() ? move(d) : a.front(); }
    T& back() {
        nassert(!empty());
        return a.back();
    }
    const T& back() const {
        nassert(!empty());
        return a.back();
    }
    T back(T d) const { return empty() ? move(d) : a.back(); }
    void del(int i) {
        nassert(0 <= i && i < len());
        a.erase(a.begin() + i);
    }
    T swapdel(int i) {
        nassert(0 <= i && i < len());
        T x = move(a[i]);
        if (i + 1 < len())
            a[i] = move(a.back());
        a.pop_back();
        return x;
    }
    nvector_stl& operator+=(const T& x) {
        push(x);
        return *this;
    }
    nvector_stl& operator+=(T&& x) {
        push(move(x));
        return *this;
    }
    struct move_cursor {
        vector<T> a;
        int i = 0;
        bool ok() const { return i < int(a.size()); }
        T& val() { return a[i]; }
        const T& val() const { return a[i]; }
        int idx() const { return i; }
        void next() { ++i; }
    };
    auto enumerate() & { return nindex_cursor<nvector_stl>{this, 0, len(), 1}; }
    auto enumerate() const& { return nindex_cursor<const nvector_stl>{this, 0, len(), 1}; }
    auto enumerate() && { return move_cursor{move(a)}; }
    friend bool operator==(const nvector_stl&, const nvector_stl&) = default;
};

template <class T, int K>
    requires(K > 0)
struct narray {
    using coord_type = array<int, K>;

  private:
    coord_type d{};
    nvector_stl<T> a;

    static int volume(const coord_type& q) {
        bool zero = false;
        nrep(i, K) {
            if (q[i] < 0) {
                nassert(false);
                return npos;
            }
            zero |= q[i] == 0;
        }
        if (zero)
            return 0;
        int n = 1;
        nrep(i, K) {
            if (n > INT_MAX / q[i]) {
                nassert(false);
                return npos;
            }
            n *= q[i];
        }
        return n;
    }

  public:
    narray() = default;
    explicit narray(coord_type q)
        requires default_initializable<T>
    {
        int n = volume(q);
        if (n == npos)
            return;
        d = q;
        a.resize(n);
    }
    narray(coord_type q, const T& x) {
        int n = volume(q);
        if (n == npos)
            return;
        d = q;
        a.resize(n, x);
    }
    template <class... I>
        requires(default_initializable<T> && sizeof...(I) == K && (same_as<remove_cvref_t<I>, int> && ...))
    explicit narray(I... q) : narray(coord_type{q...}) {}

    static constexpr int rank() { return K; }
    int dim(int k, int x = npos) const { return 0 <= k && k < K ? d[k] : x; }
    nspan<const int> shape() const& { return {d.data(), K}; }
    nspan<const int> shape() const&& = delete;
    int len() const { return a.len(); }
    bool empty() const { return a.empty(); }
    T* data() { return a.data(); }
    const T* data() const { return a.data(); }
    T& operator[](int p) { return a[p]; }
    const T& operator[](int p) const { return a[p]; }
    T* get(int p) { return a.get(p); }
    const T* get(int p) const { return a.get(p); }
    T get(int p, T x) const { return a.get(p, move(x)); }

    int pos(const coord_type& c, int x = npos) const {
        nrep(i, K)
            if (c[i] < 0 || c[i] >= d[i])
                return x;
        int p = 0;
        nrep(i, K)
            p = p * d[i] + c[i];
        return p;
    }
    template <class... I>
        requires(sizeof...(I) == K && (same_as<remove_cvref_t<I>, int> && ...))
    int pos(I... c) const {
        return pos(coord_type{c...});
    }
    T& operator()(const coord_type& c) {
        int p = pos(c);
        nassert(p != npos);
        return a[p];
    }
    const T& operator()(const coord_type& c) const {
        int p = pos(c);
        nassert(p != npos);
        return a[p];
    }
    template <class... I>
        requires(sizeof...(I) == K && (same_as<remove_cvref_t<I>, int> && ...))
    T& operator()(I... c) {
        return (*this)(coord_type{c...});
    }
    template <class... I>
        requires(sizeof...(I) == K && (same_as<remove_cvref_t<I>, int> && ...))
    const T& operator()(I... c) const {
        return (*this)(coord_type{c...});
    }
    T* get(const coord_type& c) {
        int p = pos(c);
        return p == npos ? nullptr : a.get(p);
    }
    const T* get(const coord_type& c) const {
        int p = pos(c);
        return p == npos ? nullptr : a.get(p);
    }
    T get(const coord_type& c, T x) const {
        int p = pos(c);
        return p == npos ? move(x) : a[p];
    }
    void fill(const T& x) {
        nfor(y, a)
            y = x;
    }
    auto enumerate() & { return nindex_cursor<narray>{this, 0, len(), 1}; }
    auto enumerate() const& { return nindex_cursor<const narray>{this, 0, len(), 1}; }
    auto enumerate() && = delete;
    friend bool operator==(const narray&, const narray&) = default;
};

template <class T>
    requires default_initializable<T>
struct ndeque_ring {
    vector<T> a = vector<T>(1);
    int l = 0, n = 0;
    int len() const { return n; }
    int cap() const { return int(a.size()); }
    bool empty() const { return !n; }
    int at(int i) const { return (l + i) & (cap() - 1); }
    void grow() {
        int c = cap();
        vector<T> b(c * 2);
        nrep(i, n)
            b[i] = move((*this)[i]);
        a.swap(b);
        l = 0;
    }
    T& operator[](int i) {
        nassert(0 <= i && i < n);
        return a[at(i)];
    }
    const T& operator[](int i) const {
        nassert(0 <= i && i < n);
        return a[at(i)];
    }
    T* get(int i) { return 0 <= i && i < n ? &a[at(i)] : nullptr; }
    const T* get(int i) const { return 0 <= i && i < n ? &a[at(i)] : nullptr; }
    T get(int i, T d) const { return 0 <= i && i < n ? a[at(i)] : move(d); }
    template <class U> T& pushr(U&& x) {
        if (n == cap())
            grow();
        int i = at(n++);
        return a[i] = forward<U>(x);
    }
    template <class U> T& pushl(U&& x) {
        if (n == cap())
            grow();
        l = (l - 1) & (cap() - 1);
        ++n;
        return a[l] = forward<U>(x);
    }
    T popr() {
        nassert(n);
        return move(a[at(--n)]);
    }
    T popr(T d) { return n ? popr() : move(d); }
    T popl() {
        nassert(n);
        T x = move(a[l]);
        l = (l + 1) & (cap() - 1);
        --n;
        return x;
    }
    T popl(T d) { return n ? popl() : move(d); }
    T& front() {
        nassert(n);
        return a[l];
    }
    const T& front() const {
        nassert(n);
        return a[l];
    }
    T front(T d) const { return n ? a[l] : move(d); }
    T& back() {
        nassert(n);
        return a[at(n - 1)];
    }
    const T& back() const {
        nassert(n);
        return a[at(n - 1)];
    }
    T back(T d) const { return n ? a[at(n - 1)] : move(d); }
    void clear() { l = n = 0; }
    ndeque_ring& operator+=(const T& x) {
        pushr(x);
        return *this;
    }
    ndeque_ring& operator+=(T&& x) {
        pushr(move(x));
        return *this;
    }
    auto enumerate() & { return nindex_cursor<ndeque_ring>{this, 0, n, 1}; }
    auto enumerate() const& { return nindex_cursor<const ndeque_ring>{this, 0, n, 1}; }
    auto enumerate() && = delete;
};

template <class A> struct nreverse_view {
    A* a;
    auto enumerate() const { return nindex_cursor<A>{a, a->len() - 1, -1, -1}; }
};
template <class A> auto nreverse(A& a) {
    return nreverse_view<A>{&a};
}
template <class A> auto nreverse(const A& a) {
    return nreverse_view<const A>{&a};
}
template <class A> auto nreverse(const A&&) = delete;
template <signed_integral T> struct nrange_reverse_view {
    nrange_t<T> a;
    int n = 0;
    struct cursor {
        nrange_t<T> a;
        int n, i = 0;
        bool ok() const { return i < n; }
        T val() const {
            using W = nwide_t<T>;
            return T(W(a.l) + W(a.d) * W(n - 1 - i));
        }
        int idx() const { return i; }
        void next() { ++i; }
    };
    auto enumerate() const { return cursor{a, n}; }
    int len() const { return n; }
};
template <signed_integral T> constexpr auto nreverse(nrange_t<T> a) {
    return nrange_reverse_view<T>{a, a.len()};
}

template <class A, class B> struct nzip_cursor {
    A a;
    B b;
    int i = 0;
    bool ok() const { return a.ok() && b.ok(); }
    auto val() const { return pair<decltype(a.val()), decltype(b.val())>(a.val(), b.val()); }
    int idx() const { return i; }
    void next() {
        a.next();
        b.next();
        ++i;
    }
};
template <class A, class B> struct nzip_view {
    A a;
    B b;
    auto enumerate() const { return nzip_cursor<A, B>{a, b}; }
};
template <class A, class B> auto nzip(A&& a, B&& b) {
    auto x = nenumerate(forward<A>(a)), y = nenumerate(forward<B>(b));
    return nzip_view<decltype(x), decltype(y)>{move(x), move(y)};
}

template <class A, class B> struct nproduct_cursor {
    A a;
    B b, b0;
    int i = 0;
    bool ok() const { return a.ok() && b.ok(); }
    auto val() const { return pair<decltype(a.val()), decltype(b.val())>(a.val(), b.val()); }
    int idx() const { return i; }
    void next() {
        ++i;
        b.next();
        if (!b.ok()) {
            a.next();
            b = b0;
        }
    }
};
template <class A, class B> struct nproduct_view {
    A a;
    B b;
    auto enumerate() const { return nproduct_cursor<A, B>{a, b, b}; }
};
template <class A, class B> auto nproduct(A&& a, B&& b) {
    auto x = nenumerate(forward<A>(a)), y = nenumerate(forward<B>(b));
    return nproduct_view<decltype(x), decltype(y)>{move(x), move(y)};
}

template <class A, class X> int nfind(const A& a, const X& x, int d = npos) {
    nfori(i, y, a)
        if (y == x)
            return i;
    return d;
}
template <class A, class X, class C = nless<>> int nlower(const A& a, const X& x, C c = {}) {
    int l = 0, r = a.len();
    while (l < r) {
        int m = (l + r) / 2;
        c(a[m], x) ? l = m + 1 : r = m;
    }
    return l;
}
template <class A, class X, class C = nless<>> int nupper(const A& a, const X& x, C c = {}) {
    int l = 0, r = a.len();
    while (l < r) {
        int m = (l + r) / 2;
        c(x, a[m]) ? r = m : l = m + 1;
    }
    return l;
}
template <class A, class X, class C = nless<>> int nfind_sorted(const A& a, const X& x, C c = {}, int d = npos) {
    int i = nlower(a, x, c);
    return i < a.len() && !c(x, a[i]) ? i : d;
}
template <class A, class C = nless<>>
    requires requires(A& a) {
        a.data();
        a.len();
    }
void nsort(A& a, C c = {}) {
    sort(a.data(), a.data() + a.len(), move(c));
}
template <class A> void nreverse_inplace(A& a, int l = 0, int r = npos) {
    if (r == npos)
        r = a.len();
    nassert(0 <= l && l <= r && r <= a.len());
    for (--r; l < r; ++l, --r)
        swap(a[l], a[r]);
}
template <class A, class E = nequal<>> int nunique(A& a, E eq = {}) {
    if (a.empty())
        return 0;
    int k = 1;
    for (int i = 1; i < a.len(); ++i)
        if (!eq(a[k - 1], a[i]))
            a[k++] = move(a[i]);
    a.resize(k);
    return k;
}
template <class A, class C = nless<>, class E = nequal<>> int nsort_unique(A& a, C c = {}, E eq = {}) {
    nsort(a, move(c));
    return nunique(a, move(eq));
}
template <class A, class O = nadd<remove_cvref_t<decltype(declval<const A&>()[0])>>>
auto nfold(const A& a, int l, int r, O = {}) {
    using T = remove_cvref_t<decltype(a[0])>;
    nassert(0 <= l && l <= r && r <= a.len());
    T z = O::id();
    for (int i = l; i < r; ++i)
        z = O::op(move(z), a[i]);
    return z;
}
template <class A, class O = nadd<remove_cvref_t<decltype(declval<const A&>()[0])>>> auto nfold(const A& a, O o = {}) {
    return nfold(a, 0, a.len(), move(o));
}

template <class T, class C = nless<T>> struct nheap_binary {
    vector<T> a;
    C cmp{};
    nheap_binary() = default;
    explicit nheap_binary(C c) : cmp(move(c)) {}
    template <class A> explicit nheap_binary(const A& x, C c = {}) : cmp(move(c)) {
        a.reserve(x.len());
        nfor(v, x)
            a.push_back(v);
        for (int i = int(a.size()) / 2; i--;)
            down(i);
    }
    int len() const { return int(a.size()); }
    bool empty() const { return a.empty(); }
    void clear() { a.clear(); }
    void reserve(int n) { a.reserve(n); }
    int up(int i) {
        while (i) {
            int p = (i - 1) / 2;
            if (!cmp(a[i], a[p]))
                break;
            swap(a[i], a[p]);
            i = p;
        }
        return i;
    }
    void down(int i) {
        for (int n = len(), j;; i = j) {
            j = i * 2 + 1;
            if (j >= n)
                break;
            if (j + 1 < n && cmp(a[j + 1], a[j]))
                ++j;
            if (!cmp(a[j], a[i]))
                break;
            swap(a[i], a[j]);
        }
    }
    template <class... A> T& push(A&&... x) {
        a.emplace_back(forward<A>(x)...);
        return a[up(len() - 1)];
    }
    const T& top() const {
        nassert(!empty());
        return a[0];
    }
    T top(T d) const { return empty() ? move(d) : a[0]; }
    T pop() {
        nassert(!empty());
        T z = move(a[0]);
        if (len() == 1)
            return a.pop_back(), z;
        a[0] = move(a.back());
        a.pop_back();
        down(0);
        return z;
    }
    T pop(T d) { return empty() ? move(d) : pop(); }
    void replace(T x) {
        nassert(!empty());
        a[0] = move(x);
        down(0);
    }
};

// 11 nassoc
template <class T, class C = nless<T>, bool Multi = false, class A = nempty_augment<T>>
    requires naugment<A, T>
struct nset_fhq {
    using value_type = T;
    using augment_type = A;
    using info_type = typename A::info_type;
    using node_view = nnode<nset_fhq>;

  private:
    struct node {
        T x;
        uint64_t w;
        int l = 0, r = 0, sz = 1, c = 1;
        [[no_unique_address]] info_type agg;
        node(T value, uint64_t priority, int count, info_type info)
            : x(move(value)), w(priority), sz(count), c(count), agg(move(info)) {}
    };
    npool<node> p;
    int rt = 0;
    [[no_unique_address]] C cmp{};
    [[no_unique_address]] A aug{};
    mutable uint64_t epoch = 1;

  public:
    nset_fhq() = default;
    explicit nset_fhq(C c) : cmp(move(c)) {}
    explicit nset_fhq(A a)
        requires(!same_as<C, A>)
        : aug(move(a)) {}
    nset_fhq(C c, A a) : cmp(move(c)), aug(move(a)) {}
    nset_fhq(const nset_fhq& s) : p(s.p), rt(s.rt), cmp(s.cmp), aug(s.aug) {}
    nset_fhq(nset_fhq&& s) : p(move(s.p)), rt(exchange(s.rt, 0)), cmp(move(s.cmp)), aug(move(s.aug)) { s.touch(); }
    nset_fhq& operator=(const nset_fhq& s) {
        if (this != &s) {
            p = s.p;
            rt = s.rt;
            cmp = s.cmp;
            aug = s.aug;
            touch();
        }
        return *this;
    }
    nset_fhq& operator=(nset_fhq&& s) {
        if (this != &s) {
            p = move(s.p);
            rt = exchange(s.rt, 0);
            cmp = move(s.cmp);
            aug = move(s.aug);
            touch();
            s.touch();
        }
        return *this;
    }

  private:
    void touch() const {
        if (!++epoch)
            ++epoch;
    }
    int sz(int u) const { return u ? p[u].sz : 0; }
    void pull(int u) {
        if (u) {
            p[u].sz = sz(p[u].l) + p[u].c + sz(p[u].r);
            p[u].agg = aug.op(aug.op(nnode_info(p[u].l), aug.one(p[u].x, p[u].c)), nnode_info(p[u].r));
        }
    }

    friend struct nnode<nset_fhq>;
    uint64_t nnode_epoch() const { return epoch; }
    bool nnode_alive(int u) const { return p.get(u); }
    const T& nnode_val(int u) const { return p[u].x; }
    int nnode_count(int u) const { return u ? p[u].c : 0; }
    int nnode_len(int u) const { return sz(u); }
    info_type nnode_info(int u) const { return u ? p[u].agg : aug.id(); }
    int nnode_left(int u) const { return u ? p[u].l : 0; }
    int nnode_right(int u) const { return u ? p[u].r : 0; }

    bool eq(const T& a, const T& b) const { return !cmp(a, b) && !cmp(b, a); }
    void split(int u, const T& x, int& a, int& b) {
        if (!u) {
            a = b = 0;
            return;
        }
        if (cmp(p[u].x, x))
            a = u, split(p[u].r, x, p[a].r, b), pull(a);
        else
            b = u, split(p[u].l, x, a, p[b].l), pull(b);
    }
    int add0(int u, const T& x, int c) {
        if (!u)
            return -1;
        if (eq(x, p[u].x)) {
            if constexpr (Multi)
                p[u].c += c, pull(u);
            return Multi ? c : 0;
        }
        int z = add0(cmp(x, p[u].x) ? p[u].l : p[u].r, x, c);
        if (z >= 0)
            pull(u);
        return z;
    }
    int merge(int a, int b) {
        if (!a)
            return b;
        if (!b)
            return a;
        if (p[a].w > p[b].w) {
            p[a].r = merge(p[a].r, b);
            pull(a);
            return a;
        }
        p[b].l = merge(a, p[b].l);
        pull(b);
        return b;
    }
    int del0(int& u, const T& x, int c, bool all) {
        if (!u)
            return 0;
        if (eq(x, p[u].x)) {
            if constexpr (Multi)
                if (!all && p[u].c > c) {
                    p[u].c -= c;
                    pull(u);
                    return c;
                }
            int z = Multi ? p[u].c : 1, v = u;
            u = merge(p[u].l, p[u].r);
            p.del(v);
            return z;
        }
        int z;
        if (cmp(x, p[u].x))
            z = del0(p[u].l, x, c, all);
        else
            z = del0(p[u].r, x, c, all);
        pull(u);
        return z;
    }

  public:
    int len() const { return sz(rt); }
    bool empty() const { return !rt; }
    const A& augment() const { return aug; }
    node_view root() const { return {this, rt, epoch}; }
    template <class F> node_view walk(F&& decide) const { return nwalk(*this, forward<F>(decide)); }
    template <class P> node_view first_prefix(P&& pred) const { return nfirst_prefix(*this, forward<P>(pred)); }
    template <class P> node_view last_suffix(P&& pred) const { return nlast_suffix(*this, forward<P>(pred)); }
    void reserve(int n) { p.reserve(n); }
    void clear() {
        if (rt)
            touch();
        p.clear();
        rt = 0;
    }
    int ins(const T& x, int c = 1) {
        T y = x;
        return ins(move(y), c);
    }
    int ins(T&& x, int c = 1) {
        nassert(c >= 0);
        if (!c)
            return 0;
        int z = add0(rt, x, c);
        if (z >= 0) {
            if (z)
                touch();
            return z;
        }
        int a, b;
        split(rt, x, a, b);
        int k = Multi ? c : 1;
        auto info = aug.one(x, k);
        int u = p.make(move(x), nrng_global(), k, move(info));
        rt = merge(merge(a, u), b);
        touch();
        return Multi ? c : 1;
    }
    int del(const T& x, int c = 1) {
        nassert(c >= 0);
        int z = c ? del0(rt, x, c, false) : 0;
        if (z)
            touch();
        return z;
    }
    int delall(const T& x) {
        int z = del0(rt, x, 0, true);
        if (z)
            touch();
        return z;
    }
    int findi(const T& x) const {
        for (int u = rt; u;)
            if (eq(x, p[u].x))
                return u;
            else
                u = cmp(x, p[u].x) ? p[u].l : p[u].r;
        return 0;
    }
    bool has(const T& x) const { return findi(x); }
    int count(const T& x) const {
        int u = findi(x);
        return u ? p[u].c : 0;
    }
    const T* get(const T& x) const {
        int u = findi(x);
        return u ? &p[u].x : nullptr;
    }
    int rank(const T& x) const {
        int z = 0;
        for (int u = rt; u;)
            if (cmp(p[u].x, x))
                z += sz(p[u].l) + p[u].c, u = p[u].r;
            else
                u = p[u].l;
        return z;
    }
    nmaybe<T> kth(int k) const {
        if (k < 0 || k >= len())
            return {};
        for (int u = rt; u;) {
            int l = sz(p[u].l);
            if (k < l)
                u = p[u].l;
            else if (k < l + p[u].c)
                return p[u].x;
            else
                k -= l + p[u].c, u = p[u].r;
        }
        return {};
    }
    T kth(int k, T d) const {
        auto x = kth(k);
        return x ? x.val() : move(d);
    }
    nmaybe<T> lower(const T& x) const {
        int z = 0;
        for (int u = rt; u;)
            if (!cmp(p[u].x, x))
                z = u, u = p[u].l;
            else
                u = p[u].r;
        return z ? nmaybe<T>(p[z].x) : nmaybe<T>{};
    }
    T lower(const T& x, T d) const {
        auto z = lower(x);
        return z ? z.val() : move(d);
    }
    nmaybe<T> upper(const T& x) const {
        int z = 0;
        for (int u = rt; u;)
            if (cmp(x, p[u].x))
                z = u, u = p[u].l;
            else
                u = p[u].r;
        return z ? nmaybe<T>(p[z].x) : nmaybe<T>{};
    }
    T upper(const T& x, T d) const {
        auto z = upper(x);
        return z ? z.val() : move(d);
    }
    nmaybe<T> min() const { return kth(0); }
    nmaybe<T> max() const { return kth(len() - 1); }
    T min(T d) const { return kth(0, move(d)); }
    T max(T d) const { return kth(len() - 1, move(d)); }
    struct cursor {
        const nset_fhq* s;
        vector<int> q;
        int rep = 0, k = 0;
        cursor(const nset_fhq* s) : s(s) { push(s->rt); }
        void push(int u) {
            for (; u; u = s->p[u].l)
                q.push_back(u);
        }
        bool ok() const { return !q.empty(); }
        const T& val() const { return s->p[q.back()].x; }
        int idx() const { return k; }
        void next() {
            int u = q.back();
            ++k;
            if (++rep < s->p[u].c)
                return;
            rep = 0;
            q.pop_back();
            push(s->p[u].r);
        }
    };
    auto enumerate() const& { return cursor(this); }
    auto enumerate() && = delete;
    nset_fhq& operator|=(const nset_fhq& b)
        requires(!Multi)
    {
        if (this == &b)
            return *this;
        nfor(x, b)
            ins(x);
        return *this;
    }
    nset_fhq& operator&=(const nset_fhq& b)
        requires(!Multi)
    {
        if (this == &b)
            return *this;
        vector<T> d;
        nfor(x, *this)
            if (!b.has(x))
                d.push_back(x);
        for (auto& x : d)
            del(x);
        return *this;
    }
    nset_fhq& operator-=(const nset_fhq& b)
        requires(!Multi)
    {
        if (this == &b)
            return clear(), *this;
        nfor(x, b)
            del(x);
        return *this;
    }
    nset_fhq& operator^=(const nset_fhq& b)
        requires(!Multi)
    {
        if (this == &b)
            return clear(), *this;
        nfor(x, b)
            has(x) ? del(x) : ins(x);
        return *this;
    }
    friend nset_fhq operator|(nset_fhq a, const nset_fhq& b)
        requires(!Multi)
    {
        return a |= b;
    }
    friend nset_fhq operator&(nset_fhq a, const nset_fhq& b)
        requires(!Multi)
    {
        return a &= b;
    }
    friend nset_fhq operator-(nset_fhq a, const nset_fhq& b)
        requires(!Multi)
    {
        return a -= b;
    }
    friend nset_fhq operator^(nset_fhq a, const nset_fhq& b)
        requires(!Multi)
    {
        return a ^= b;
    }
    friend bool operator==(const nset_fhq& a, const nset_fhq& b) {
        if (a.len() != b.len())
            return false;
        auto x = a.enumerate(), y = b.enumerate();
        for (; x.ok(); x.next(), y.next())
            if (!a.eq(x.val(), y.val()))
                return false;
        return true;
    }
};

template <class T, class C = nless<T>, bool Multi = false, class A = nempty_augment<T>>
    requires naugment<A, T>
struct nset_splay {
    using value_type = T;
    using augment_type = A;
    using info_type = typename A::info_type;
    using node_view = nnode<nset_splay>;

  private:
    struct node {
        T x;
        int l = 0, r = 0, p = 0, sz = 1, c = 1;
        [[no_unique_address]] info_type agg;
        node(T value, int count, info_type info) : x(move(value)), sz(count), c(count), agg(move(info)) {}
    };
    mutable npool<node> p;
    mutable int rt = 0;
    [[no_unique_address]] C cmp{};
    [[no_unique_address]] A aug{};
    mutable uint64_t epoch = 1;

  public:
    nset_splay() = default;
    explicit nset_splay(C c) : cmp(move(c)) {}
    explicit nset_splay(A a)
        requires(!same_as<C, A>)
        : aug(move(a)) {}
    nset_splay(C c, A a) : cmp(move(c)), aug(move(a)) {}
    nset_splay(const nset_splay& s) : p(s.p), rt(s.rt), cmp(s.cmp), aug(s.aug) {}
    nset_splay(nset_splay&& s) : p(move(s.p)), rt(exchange(s.rt, 0)), cmp(move(s.cmp)), aug(move(s.aug)) { s.touch(); }
    nset_splay& operator=(const nset_splay& s) {
        if (this != &s) {
            p = s.p;
            rt = s.rt;
            cmp = s.cmp;
            aug = s.aug;
            touch();
        }
        return *this;
    }
    nset_splay& operator=(nset_splay&& s) {
        if (this != &s) {
            p = move(s.p);
            rt = exchange(s.rt, 0);
            cmp = move(s.cmp);
            aug = move(s.aug);
            touch();
            s.touch();
        }
        return *this;
    }

  private:
    void touch() const {
        if (!++epoch)
            ++epoch;
    }
    int sz(int u) const { return u ? p[u].sz : 0; }
    void pull(int u) const {
        if (u) {
            p[u].sz = sz(p[u].l) + p[u].c + sz(p[u].r);
            p[u].agg = aug.op(aug.op(nnode_info(p[u].l), aug.one(p[u].x, p[u].c)), nnode_info(p[u].r));
        }
    }

    friend struct nnode<nset_splay>;
    uint64_t nnode_epoch() const { return epoch; }
    bool nnode_alive(int u) const { return p.get(u); }
    const T& nnode_val(int u) const { return p[u].x; }
    int nnode_count(int u) const { return u ? p[u].c : 0; }
    int nnode_len(int u) const { return sz(u); }
    info_type nnode_info(int u) const { return u ? p[u].agg : aug.id(); }
    int nnode_left(int u) const { return u ? p[u].l : 0; }
    int nnode_right(int u) const { return u ? p[u].r : 0; }

    bool eq(const T& a, const T& b) const { return !cmp(a, b) && !cmp(b, a); }
    int& ch(int u, bool r) const { return r ? p[u].r : p[u].l; }
    void rot(int x) const {
        int y = p[x].p, z = p[y].p;
        bool d = x == p[y].r;
        int b = ch(x, !d);
        if (z)
            ch(z, y == p[z].r) = x;
        else
            rt = x;
        p[x].p = z;
        ch(y, d) = b;
        if (b)
            p[b].p = y;
        ch(x, !d) = y;
        p[y].p = x;
        pull(y);
        pull(x);
    }
    void splay(int x) const {
        if (p[x].p)
            touch();
        while (p[x].p) {
            int y = p[x].p, z = p[y].p;
            if (z)
                rot((x == p[y].r) == (y == p[z].r) ? y : x);
            rot(x);
        }
    }

  public:
    int len() const { return sz(rt); }
    bool empty() const { return !rt; }
    const A& augment() const { return aug; }
    node_view root() const { return {this, rt, epoch}; }
    template <class F> node_view walk(F&& decide) const { return nwalk(*this, forward<F>(decide)); }
    template <class P> node_view first_prefix(P&& pred) const { return nfirst_prefix(*this, forward<P>(pred)); }
    template <class P> node_view last_suffix(P&& pred) const { return nlast_suffix(*this, forward<P>(pred)); }
    void reserve(int n) { p.reserve(n); }
    void clear() {
        if (rt)
            touch();
        p.clear();
        rt = 0;
    }
    int findi(const T& x) const {
        int u = rt, z = 0;
        while (u) {
            z = u;
            if (eq(x, p[u].x))
                break;
            u = ch(u, cmp(p[u].x, x));
        }
        if (z)
            splay(z);
        return z && eq(x, p[z].x) ? z : 0;
    }
    bool has(const T& x) const { return findi(x); }
    int count(const T& x) const {
        int u = findi(x);
        return u ? p[u].c : 0;
    }
    const T* get(const T& x) const {
        int u = findi(x);
        return u ? &p[u].x : nullptr;
    }
    int ins(const T& x, int c = 1) {
        T y = x;
        return ins(move(y), c);
    }
    int ins(T&& x, int c = 1) {
        nassert(c >= 0);
        if (!c)
            return 0;
        if (!rt) {
            int k = Multi ? c : 1;
            auto info = aug.one(x, k);
            rt = p.make(move(x), k, move(info));
            touch();
            return k;
        }
        int u = rt, v = 0;
        while (u) {
            v = u;
            if (eq(x, p[u].x)) {
                splay(u);
                if constexpr (Multi) {
                    p[u].c += c, pull(u);
                    touch();
                }
                return Multi ? c : 0;
            }
            u = ch(u, cmp(p[u].x, x));
        }
        int k = Multi ? c : 1;
        auto info = aug.one(x, k);
        int z = p.make(move(x), k, move(info));
        p[z].p = v;
        ch(v, cmp(p[v].x, p[z].x)) = z;
        splay(z);
        return Multi ? c : 1;
    }
    int del(const T& x, int c = 1) {
        nassert(c >= 0);
        if (!c)
            return 0;
        int u = findi(x);
        if (!u)
            return 0;
        if constexpr (Multi)
            if (p[u].c > c) {
                p[u].c -= c;
                pull(u);
                touch();
                return c;
            }
        int z = Multi ? p[u].c : 1, l = p[u].l, r = p[u].r;
        touch();
        p.del(u);
        if (!l) {
            rt = r;
            if (r)
                p[r].p = 0;
            return z;
        }
        rt = l;
        p[l].p = 0;
        int v = l;
        while (p[v].r)
            v = p[v].r;
        splay(v);
        p[rt].r = r;
        if (r)
            p[r].p = rt;
        pull(rt);
        return z;
    }
    int delall(const T& x) {
        int c = count(x);
        return c ? del(x, c) : 0;
    }
    int rank(const T& x) const {
        int u = rt, z = 0, k = 0;
        while (u) {
            z = u;
            if (cmp(p[u].x, x))
                k += sz(p[u].l) + p[u].c, u = p[u].r;
            else
                u = p[u].l;
        }
        if (z)
            splay(z);
        return k;
    }
    nmaybe<T> kth(int k) const {
        if (k < 0 || k >= len())
            return {};
        int u = rt;
        for (;;) {
            int l = sz(p[u].l);
            if (k < l)
                u = p[u].l;
            else if (k < l + p[u].c) {
                splay(u);
                return p[u].x;
            } else
                k -= l + p[u].c, u = p[u].r;
        }
    }
    T kth(int k, T d) const {
        auto z = kth(k);
        return z ? z.val() : move(d);
    }
    nmaybe<T> bound(const T& x, bool up) const {
        int u = rt, z = 0, last = 0;
        while (u) {
            last = u;
            if (up ? cmp(x, p[u].x) : !cmp(p[u].x, x))
                z = u, u = p[u].l;
            else
                u = p[u].r;
        }
        if (z)
            splay(z);
        else if (last)
            splay(last);
        return z ? nmaybe<T>(p[z].x) : nmaybe<T>{};
    }
    nmaybe<T> lower(const T& x) const { return bound(x, false); }
    nmaybe<T> upper(const T& x) const { return bound(x, true); }
    T lower(const T& x, T d) const {
        auto z = lower(x);
        return z ? z.val() : move(d);
    }
    T upper(const T& x, T d) const {
        auto z = upper(x);
        return z ? z.val() : move(d);
    }
    nmaybe<T> min() const { return kth(0); }
    nmaybe<T> max() const { return kth(len() - 1); }
    T min(T d) const { return kth(0, move(d)); }
    T max(T d) const { return kth(len() - 1, move(d)); }
    struct cursor {
        const nset_splay* s;
        vector<int> q;
        int rep = 0, k = 0;
        cursor(const nset_splay* x) : s(x) { push(x->rt); }
        void push(int u) {
            while (u)
                q.push_back(u), u = s->p[u].l;
        }
        bool ok() const { return !q.empty(); }
        const T& val() const { return s->p[q.back()].x; }
        int idx() const { return k; }
        void next() {
            int u = q.back();
            ++k;
            if (++rep < s->p[u].c)
                return;
            rep = 0;
            q.pop_back();
            push(s->p[u].r);
        }
    };
    auto enumerate() const& { return cursor(this); }
    auto enumerate() && = delete;
    nset_splay& operator|=(const nset_splay& b)
        requires(!Multi)
    {
        if (this != &b) {
            nfor(x, b)
                ins(x);
        }
        return *this;
    }
    nset_splay& operator&=(const nset_splay& b)
        requires(!Multi)
    {
        if (this == &b)
            return *this;
        vector<T> d;
        nfor(x, *this)
            if (!b.has(x))
                d.push_back(x);
        for (auto& x : d)
            del(x);
        return *this;
    }
    nset_splay& operator-=(const nset_splay& b)
        requires(!Multi)
    {
        if (this == &b)
            return clear(), *this;
        nfor(x, b)
            del(x);
        return *this;
    }
    nset_splay& operator^=(const nset_splay& b)
        requires(!Multi)
    {
        if (this == &b)
            return clear(), *this;
        nfor(x, b)
            has(x) ? del(x) : ins(x);
        return *this;
    }
    friend nset_splay operator|(nset_splay a, const nset_splay& b)
        requires(!Multi)
    {
        return a |= b;
    }
    friend nset_splay operator&(nset_splay a, const nset_splay& b)
        requires(!Multi)
    {
        return a &= b;
    }
    friend nset_splay operator-(nset_splay a, const nset_splay& b)
        requires(!Multi)
    {
        return a -= b;
    }
    friend nset_splay operator^(nset_splay a, const nset_splay& b)
        requires(!Multi)
    {
        return a ^= b;
    }
    friend bool operator==(const nset_splay& a, const nset_splay& b) {
        if (a.len() != b.len())
            return false;
        auto x = a.enumerate(), y = b.enumerate();
        while (x.ok()) {
            if (!a.eq(x.val(), y.val()))
                return false;
            x.next();
            y.next();
        }
        return true;
    }
};

template <class T, class C = nless<T>> struct nset_stl {
    set<T, C> a;
    nset_stl() = default;
    nset_stl(initializer_list<T> x) : a(x) {}
    bool eq(const T& x, const T& y) const {
        auto c = a.key_comp();
        return !c(x, y) && !c(y, x);
    }
    int len() const { return int(a.size()); }
    bool empty() const { return a.empty(); }
    void clear() { a.clear(); }
    void reserve(int) {}
    int ins(const T& x, int = 1) { return a.insert(x).second; }
    int ins(T&& x, int = 1) { return a.insert(move(x)).second; }
    int del(const T& x, int = 1) { return int(a.erase(x)); }
    int delall(const T& x) { return del(x); }
    bool has(const T& x) const { return a.contains(x); }
    int count(const T& x) const { return has(x); }
    const T* get(const T& x) const {
        auto i = a.find(x);
        return i == a.end() ? nullptr : &*i;
    }
    int rank(const T& x) const {
        auto e = a.lower_bound(x);
        auto i = a.begin();
        int z = 0;
        for (; i != e; ++i)
            ++z;
        return z;
    }
    nmaybe<T> kth(int k) const {
        if (k < 0 || k >= len())
            return {};
        auto i = a.begin();
        for (int j = 0; j < k; ++j)
            ++i;
        return *i;
    }
    T kth(int k, T d) const {
        auto x = kth(k);
        return x ? x.val() : move(d);
    }
    nmaybe<T> lower(const T& x) const {
        auto i = a.lower_bound(x);
        return i == a.end() ? nmaybe<T>{} : nmaybe<T>(*i);
    }
    T lower(const T& x, T d) const {
        auto z = lower(x);
        return z ? z.val() : move(d);
    }
    nmaybe<T> upper(const T& x) const {
        auto i = a.upper_bound(x);
        return i == a.end() ? nmaybe<T>{} : nmaybe<T>(*i);
    }
    T upper(const T& x, T d) const {
        auto z = upper(x);
        return z ? z.val() : move(d);
    }
    nmaybe<T> min() const { return kth(0); }
    nmaybe<T> max() const { return kth(len() - 1); }
    T min(T d) const { return kth(0, move(d)); }
    T max(T d) const { return kth(len() - 1, move(d)); }
    struct cursor {
        const nset_stl* s;
        typename set<T, C>::const_iterator i;
        int k = 0;
        bool ok() const { return i != s->a.end(); }
        const T& val() const { return *i; }
        int idx() const { return k; }
        void next() {
            ++i;
            ++k;
        }
    };
    auto enumerate() const& { return cursor{this, a.begin()}; }
    auto enumerate() && = delete;
    nset_stl& operator|=(const nset_stl& b) {
        if (this != &b) {
            nfor(x, b)
                ins(x);
        }
        return *this;
    }
    nset_stl& operator&=(const nset_stl& b) {
        if (this == &b)
            return *this;
        vector<T> d;
        nfor(x, *this)
            if (!b.has(x))
                d.push_back(x);
        for (auto& x : d)
            del(x);
        return *this;
    }
    nset_stl& operator-=(const nset_stl& b) {
        if (this == &b)
            return clear(), *this;
        nfor(x, b)
            del(x);
        return *this;
    }
    nset_stl& operator^=(const nset_stl& b) {
        if (this == &b)
            return clear(), *this;
        nfor(x, b)
            has(x) ? del(x) : ins(x);
        return *this;
    }
    friend nset_stl operator|(nset_stl a, const nset_stl& b) { return a |= b; }
    friend nset_stl operator&(nset_stl a, const nset_stl& b) { return a &= b; }
    friend nset_stl operator-(nset_stl a, const nset_stl& b) { return a -= b; }
    friend nset_stl operator^(nset_stl a, const nset_stl& b) { return a ^= b; }
    friend bool operator==(const nset_stl& a, const nset_stl& b) { return a.a == b.a; }
};

template <class K, class V, class H = nhash<K>, class E = equal_to<K>> struct nmap_hash {
    using base = unordered_map<K, V, H, E>;
    base a;
    nmap_hash() = default;
    explicit nmap_hash(int n) { reserve(n); }
    int len() const { return int(a.size()); }
    bool empty() const { return a.empty(); }
    void clear() { a.clear(); }
    void reserve(int n) { a.reserve(n); }
    bool has(const K& k) const { return a.contains(k); }
    V* get(const K& k) {
        auto i = a.find(k);
        return i == a.end() ? nullptr : &i->second;
    }
    const V* get(const K& k) const {
        auto i = a.find(k);
        return i == a.end() ? nullptr : &i->second;
    }
    V get(const K& k, V d) const {
        auto p = get(k);
        return p ? *p : move(d);
    }
    bool ins(const K& k, const V& v) { return a.emplace(k, v).second; }
    bool ins(K&& k, V&& v) { return a.emplace(move(k), move(v)).second; }
    template <class X> V& set(const K& k, X&& v) { return a.insert_or_assign(k, forward<X>(v)).first->second; }
    int del(const K& k) { return int(a.erase(k)); }
    V& operator[](const K& k) { return a[k]; }
    V& operator[](K&& k) { return a[move(k)]; }
    V& operator()(const K& k) {
        auto p = get(k);
        nassert(p);
        return *p;
    }
    const V& operator()(const K& k) const {
        auto p = get(k);
        nassert(p);
        return *p;
    }
    struct cursor {
        nmap_hash* m;
        typename base::iterator i;
        int k = 0;
        bool ok() const { return i != m->a.end(); }
        const K& key() const { return i->first; }
        V& val() const { return i->second; }
        int idx() const { return k; }
        void next() {
            ++i;
            ++k;
        }
    };
    struct ccursor {
        const nmap_hash* m;
        typename base::const_iterator i;
        int k = 0;
        bool ok() const { return i != m->a.end(); }
        const K& key() const { return i->first; }
        const V& val() const { return i->second; }
        int idx() const { return k; }
        void next() {
            ++i;
            ++k;
        }
    };
    auto enumerate() & { return cursor{this, a.begin()}; }
    auto enumerate() const& { return ccursor{this, a.begin()}; }
    auto enumerate() && = delete;
    friend bool operator==(const nmap_hash& a, const nmap_hash& b) { return a.a == b.a; }
};

template <class K, class V, class H = nhash<K>, class E = equal_to<K>> struct nmap_flat {
    struct node {
        K k;
        V v;
        size_t h;
    };
    vector<node> a;
    vector<int> b;
    int dead = 0;
    [[no_unique_address]] H hs{};
    [[no_unique_address]] E eq{};
    static int capof(uint64_t q) {
        q = max<uint64_t>(8, q);
        nassert(q <= uint64_t(1) << 30);
        if (q > uint64_t(1) << 30)
            return 1 << 30;
        return int(bit_ceil(uint32_t(q)));
    }
    nmap_flat() = default;
    explicit nmap_flat(int n, H h = {}, E e = {}) : hs(move(h)), eq(move(e)) { reserve(n); }
    int len() const { return int(a.size()); }
    int cap() const { return int(b.size()); }
    bool empty() const { return a.empty(); }
    void place(int i) {
        int m = cap() - 1, j = int(a[i].h & size_t(m));
        while (b[j])
            j = (j + 1) & m;
        b[j] = i + 1;
    }
    void rehash(int c) {
        c = capof(unsigned(max(8, c)));
        b.assign(c, 0);
        dead = 0;
        nrep(i, len())
            place(i);
    }
    void reserve(int n) {
        nassert(n >= 0);
        if (n < 0)
            return;
        uint64_t q = (10ULL * uint64_t(n) + 6) / 7;
        if (q > uint64_t(1) << 30) {
            nassert(false);
            return;
        }
        a.reserve(size_t(n));
        int c = capof(q);
        if (c > cap())
            rehash(c);
    }
    void ensure() {
        if (!cap())
            rehash(8);
        else if ((static_cast<long long>(len()) + dead + 1) * 10 > static_cast<long long>(cap()) * 7) {
            if (cap() > (1 << 29)) {
                nassert(false);
                return;
            }
            rehash(dead > len() ? cap() : cap() * 2);
        }
    }
    int slot(const K& k, size_t h, bool& ok) const {
        if (!cap())
            return ok = false, 0;
        int m = cap() - 1, i = int(h & size_t(m)), d = npos;
        for (;; i = (i + 1) & m) {
            int z = b[i];
            if (!z)
                return ok = false, d == npos ? i : d;
            if (z < 0) {
                if (d == npos)
                    d = i;
            } else if (a[z - 1].h == h && eq(a[z - 1].k, k))
                return ok = true, i;
        }
    }
    V* get(const K& k) {
        bool ok;
        int i = slot(k, hs(k), ok);
        return ok ? &a[b[i] - 1].v : nullptr;
    }
    const V* get(const K& k) const {
        bool ok;
        int i = slot(k, hs(k), ok);
        return ok ? &a[b[i] - 1].v : nullptr;
    }
    V get(const K& k, V d) const {
        auto p = get(k);
        return p ? *p : move(d);
    }
    bool has(const K& k) const { return get(k); }
    template <class X, class Y> bool ins0(X&& k, Y&& v) {
        ensure();
        size_t h = hs(k);
        bool ok;
        int i = slot(k, h, ok);
        if (ok)
            return false;
        if (b[i] < 0)
            --dead;
        a.push_back({forward<X>(k), forward<Y>(v), h});
        b[i] = len();
        return true;
    }
    bool ins(const K& k, const V& v) { return ins0(k, v); }
    bool ins(K&& k, V&& v) { return ins0(move(k), move(v)); }
    template <class X> V& set(const K& k, X&& v) {
        if (auto p = get(k))
            return *p = forward<X>(v);
        ins0(k, forward<X>(v));
        return a.back().v;
    }
    V& operator[](const K& k) {
        if (auto p = get(k))
            return *p;
        ins0(k, V{});
        return a.back().v;
    }
    V& operator[](K&& k) {
        if (auto p = get(k))
            return *p;
        ins0(move(k), V{});
        return a.back().v;
    }
    V& operator()(const K& k) {
        auto p = get(k);
        nassert(p);
        return *p;
    }
    const V& operator()(const K& k) const {
        auto p = get(k);
        nassert(p);
        return *p;
    }
    int del(const K& k) {
        if (!cap())
            return 0;
        size_t h = hs(k);
        bool ok;
        int s = slot(k, h, ok);
        if (!ok)
            return 0;
        int i = b[s] - 1, j = len() - 1;
        b[s] = -1;
        ++dead;
        if (i != j) {
            a[i] = move(a[j]);
            int m = cap() - 1, q = int(a[i].h & size_t(m));
            while (b[q] != j + 1)
                q = (q + 1) & m;
            b[q] = i + 1;
        }
        a.pop_back();
        if (dead > len() && cap() > 8)
            rehash(cap());
        return 1;
    }
    void clear() {
        a.clear();
        fill(b.begin(), b.end(), 0);
        dead = 0;
    }
    struct cursor {
        nmap_flat* m;
        int i = 0;
        bool ok() const { return i < m->len(); }
        const K& key() const { return m->a[i].k; }
        V& val() const { return m->a[i].v; }
        int idx() const { return i; }
        void next() { ++i; }
    };
    struct ccursor {
        const nmap_flat* m;
        int i = 0;
        bool ok() const { return i < m->len(); }
        const K& key() const { return m->a[i].k; }
        const V& val() const { return m->a[i].v; }
        int idx() const { return i; }
        void next() { ++i; }
    };
    auto enumerate() & { return cursor{this}; }
    auto enumerate() const& { return ccursor{this}; }
    auto enumerate() && = delete;
    friend bool operator==(const nmap_flat& x, const nmap_flat& y) {
        if (x.len() != y.len())
            return false;
        for (auto& v : x.a) {
            auto p = y.get(v.k);
            if (!p || !(*p == v.v))
                return false;
        }
        return true;
    }
};
template <class K, class V, class H = nhash<K>, class E = equal_to<K>> using nmap_stl = nmap_hash<K, V, H, E>;

// 12 nfinite
template <class A, class B, class EA = nequal<>, class EB = nequal<>> struct nrel_scan {
    struct edge {
        A a;
        B b;
    };

  private:
    vector<edge> e;
    EA ea{};
    EB eb{};

  public:
    nrel_scan() = default;
    explicit nrel_scan(EA x, EB y = {}) : ea(move(x)), eb(move(y)) {}
    int len() const { return int(e.size()); }
    bool empty() const { return e.empty(); }
    void reserve(int n) { e.reserve(n); }
    void clear() { e.clear(); }
    bool eql(const A& x, const A& y) const { return ea(x, y); }
    bool eqr(const B& x, const B& y) const { return eb(x, y); }
    bool has(const A& a, const B& b) const {
        for (auto& x : e)
            if (eql(x.a, a) && eqr(x.b, b))
                return true;
        return false;
    }
    bool add(const A& a, const B& b) {
        if (has(a, b))
            return false;
        e.push_back({a, b});
        return true;
    }
    bool add(A&& a, B&& b) {
        if (has(a, b))
            return false;
        e.push_back({move(a), move(b)});
        return true;
    }
    bool del(const A& a, const B& b) {
        for (int i = 0; i < len(); ++i)
            if (eql(e[i].a, a) && eqr(e[i].b, b)) {
                if (i + 1 < len())
                    e[i] = move(e.back());
                e.pop_back();
                return true;
            }
        return false;
    }
    nvector_stl<B> image(const A& a) const {
        nvector_stl<B> z;
        for (auto& x : e)
            if (eql(x.a, a))
                z.push(x.b);
        return z;
    }
    nvector_stl<A> preimage(const B& b) const {
        nvector_stl<A> z;
        for (auto& x : e)
            if (eqr(x.b, b))
                z.push(x.a);
        return z;
    }
    struct cursor {
        const nrel_scan* s;
        int i = 0;
        bool ok() const { return i < s->len(); }
        auto val() const { return pair<const A&, const B&>(s->e[i].a, s->e[i].b); }
        int idx() const { return i; }
        void next() { ++i; }
    };
    auto enumerate() const& { return cursor{this}; }
    auto enumerate() && = delete;
    nrel_scan& operator|=(const nrel_scan& b) {
        if (this != &b) {
            nfor(x, b)
                add(x.first, x.second);
        }
        return *this;
    }
    nrel_scan& operator&=(const nrel_scan& b) {
        if (this == &b)
            return *this;
        vector<edge> d;
        nfor(x, *this)
            if (b.has(x.first, x.second))
                d.push_back({x.first, x.second});
        e.swap(d);
        return *this;
    }
    nrel_scan& operator-=(const nrel_scan& b) {
        if (this == &b)
            return clear(), *this;
        nfor(x, b)
            del(x.first, x.second);
        return *this;
    }
    nrel_scan& operator^=(const nrel_scan& b) {
        if (this == &b)
            return clear(), *this;
        nfor(x, b)
            has(x.first, x.second) ? del(x.first, x.second) : add(x.first, x.second);
        return *this;
    }
    friend nrel_scan operator|(nrel_scan a, const nrel_scan& b) { return a |= b; }
    friend nrel_scan operator&(nrel_scan a, const nrel_scan& b) { return a &= b; }
    friend nrel_scan operator-(nrel_scan a, const nrel_scan& b) { return a -= b; }
    friend nrel_scan operator^(nrel_scan a, const nrel_scan& b) { return a ^= b; }
    friend bool operator==(const nrel_scan& a, const nrel_scan& b) {
        if (a.len() != b.len())
            return false;
        for (auto& x : a.e)
            if (!b.has(x.a, x.b))
                return false;
        return true;
    }
};
template <class A, class B, class EA = nequal<>, class EB = nequal<>> using nrel = nrel_scan<A, B, EA, EB>;

template <class A, class B, class HA = nhash<A>, class HB = nhash<B>, class EA = equal_to<A>, class EB = equal_to<B>>
struct nfunc_hash {
    nmap_flat<A, B, HA, EA> f;
    EB eb{};
    nfunc_hash() = default;
    explicit nfunc_hash(EB e) : eb(move(e)) {}
    int len() const { return f.len(); }
    bool empty() const { return f.empty(); }
    void reserve(int n) { f.reserve(n); }
    void clear() { f.clear(); }
    bool has(const A& a) const { return f.has(a); }
    B* to(const A& a) { return f.get(a); }
    const B* to(const A& a) const { return f.get(a); }
    B to(const A& a, B d) const { return f.get(a, move(d)); }
    bool bind(const A& a, const B& b) {
        auto p = f.get(a);
        return p ? p && eb(*p, b) : f.ins(a, b);
    }
    void set(const A& a, const B& b) { f.set(a, b); }
    bool unbind(const A& a) { return f.del(a); }
    B& operator()(const A& a) {
        auto p = to(a);
        nassert(p);
        return *p;
    }
    const B& operator()(const A& a) const {
        auto p = to(a);
        nassert(p);
        return *p;
    }
    auto enumerate() const& { return f.enumerate(); }
    auto enumerate() && = delete;
    friend bool operator==(const nfunc_hash& a, const nfunc_hash& b) { return a.f == b.f; }
};
template <class A, class B, class HA = nhash<A>, class HB = nhash<B>, class EA = equal_to<A>, class EB = equal_to<B>>
using nfunc = nfunc_hash<A, B, HA, HB, EA, EB>;

template <class A, class B, class HA = nhash<A>, class HB = nhash<B>, class EA = equal_to<A>, class EB = equal_to<B>>
struct nbije_hash {
    template <class, class, class, class, class, class> friend struct nbije_hash;

  private:
    nmap_flat<A, B, HA, EA> f;
    nmap_flat<B, A, HB, EB> g;
    EA ea{};
    EB eb{};

  public:
    int len() const { return f.len(); }
    bool empty() const { return f.empty(); }
    void reserve(int n) {
        f.reserve(n);
        g.reserve(n);
    }
    void clear() {
        f.clear();
        g.clear();
    }
    bool hasl(const A& a) const { return f.has(a); }
    bool hasr(const B& b) const { return g.has(b); }
    const B* to(const A& a) const { return f.get(a); }
    const A* from(const B& b) const { return g.get(b); }
    B to(const A& a, B d) const { return f.get(a, move(d)); }
    A from(const B& b, A d) const { return g.get(b, move(d)); }
    const B& operator()(const A& a) const {
        auto p = to(a);
        nassert(p);
        return *p;
    }
    bool bind(const A& a, const B& b) {
        auto x = to(a);
        auto y = from(b);
        if (x || y)
            return x && y && eb(*x, b) && ea(*y, a);
        f.ins(a, b);
        g.ins(b, a);
        return true;
    }
    bool unbindl(const A& a) {
        auto b = to(a);
        if (!b)
            return false;
        B x = *b;
        f.del(a);
        g.del(x);
        return true;
    }
    bool unbindr(const B& b) {
        auto a = from(b);
        if (!a)
            return false;
        A x = *a;
        g.del(b);
        f.del(x);
        return true;
    }
    void set(const A& a, const B& b) {
        unbindl(a);
        unbindr(b);
        bool z = bind(a, b);
        nassert(z);
    }
    auto inverse() const {
        nbije_hash<B, A, HB, HA, EB, EA> z;
        z.f = g;
        z.g = f;
        z.ea = eb;
        z.eb = ea;
        return z;
    }
    auto operator~() const { return inverse(); }
    auto enumerate() const& { return f.enumerate(); }
    auto enumerate() && = delete;
    template <class X, class HX, class EX> auto operator*(const nbije_hash<X, A, HX, HA, EX, EA>& h) const {
        nbije_hash<X, B, HX, HB, EX, EB> z;
        nforkv(x, a, h)
            if (auto b = to(a))
                z.bind(x, *b);
        return z;
    }
    friend bool operator==(const nbije_hash& a, const nbije_hash& b) { return a.f == b.f; }
};

template <class T, class C = nless<T>> struct nbije_rank {
  private:
    vector<T> a;
    C cmp{};

  public:
    nbije_rank() = default;
    explicit nbije_rank(vector<T> x, C c = {}) : a(move(x)), cmp(move(c)) {
        sort(a.begin(), a.end(), cmp);
        a.erase(unique(a.begin(), a.end(), [&](auto& x, auto& y) { return !cmp(x, y) && !cmp(y, x); }), a.end());
    }
    int len() const { return int(a.size()); }
    bool empty() const { return a.empty(); }
    int to(const T& x) const {
        int i = int(lower_bound(a.begin(), a.end(), x, cmp) - a.begin());
        return i < len() && !cmp(x, a[i]) && !cmp(a[i], x) ? i : npos;
    }
    int to(const T& x, int d) const {
        int i = to(x);
        return i == npos ? d : i;
    }
    bool hasl(const T& x) const { return to(x) != npos; }
    bool hasr(int i) const { return 0 <= i && i < len(); }
    const T* from(int i) const { return hasr(i) ? &a[i] : nullptr; }
    T from(int i, T d) const { return hasr(i) ? a[i] : move(d); }
    const T& operator()(int i) const {
        nassert(hasr(i));
        return a[i];
    }
    struct cursor {
        const nbije_rank* s;
        int i = 0;
        bool ok() const { return i < s->len(); }
        const T& key() const { return s->a[i]; }
        int val() const { return i; }
        int idx() const { return i; }
        void next() { ++i; }
    };
    auto enumerate() const& { return cursor{this}; }
    auto enumerate() && = delete;
    auto inverse() const {
        nbije_hash<int, T> z;
        z.reserve(len());
        nrep(i, len())
            z.bind(i, a[i]);
        return z;
    }
    auto operator~() const { return inverse(); }
};
template <class A, class C = nless<typename A::value_type>> auto ncompress_stl(const A& a, C c = {}) {
    using T = typename A::value_type;
    return nbije_rank<T, C>(vector<T>(a.begin(), a.end()), move(c));
}
template <class A, class C = nless<remove_cvref_t<decltype(nenumerate(declval<const A&>()).val())>>>
auto ncompress(const A& a, C c = {}) {
    using T = remove_cvref_t<decltype(nenumerate(a).val())>;
    vector<T> v;
    v.reserve(a.len());
    nfor(x, a)
        v.push_back(x);
    return nbije_rank<T, C>(move(v), move(c));
}

struct nperm {
  private:
    nvector_stl<int> p, q;

  public:
    nperm() = default;
    explicit nperm(int n) : p(n >= 0 ? n : 0), q(n >= 0 ? n : 0) {
        nassert(n >= 0);
        nrep(i, p.len())
            p[i] = q[i] = i;
    }
    explicit nperm(nvector_stl<int> x) : p(move(x)), q(p.len(), npos) {
        nrep(i, p.len()) {
            nassert(0 <= p[i] && p[i] < p.len() && q[p[i]] == npos);
            q[p[i]] = i;
        }
    }
    int len() const { return p.len(); }
    bool empty() const { return p.empty(); }
    int operator()(int i) const {
        nassert(0 <= i && i < len());
        return p[i];
    }
    int get(int i, int d = npos) const { return 0 <= i && i < len() ? p[i] : d; }
    int inv(int i, int d = npos) const { return 0 <= i && i < len() ? q[i] : d; }
    nperm inverse() const {
        nperm z;
        z.p = q;
        z.q = p;
        return z;
    }
    nperm operator~() const { return inverse(); }
    nperm& operator*=(const nperm& b) {
        nassert(len() == b.len());
        nvector_stl<int> x(len()), y(len());
        nrep(i, len())
            x[i] = p[b.p[i]], y[x[i]] = i;
        p = move(x);
        q = move(y);
        return *this;
    }
    friend nperm operator*(nperm a, const nperm& b) { return a *= b; }
    nperm pow(long long k) const {
        nperm a = k < 0 ? ~*this : *this, r(len());
        uint64_t e = k < 0 ? 0 - uint64_t(k) : uint64_t(k);
        for (; e; e >>= 1, a *= a)
            if (e & 1)
                r *= a;
        return r;
    }
    int sign() const {
        vector<char> v(len());
        int c = 0;
        nrep(i, len())
            if (!v[i]) {
                ++c;
                for (int j = i; !v[j]; j = p[j])
                    v[j] = 1;
            }
        return ((len() - c) & 1) ? -1 : 1;
    }
    auto enumerate() const& { return p.enumerate(); }
    auto enumerate() && = delete;
    friend bool operator==(const nperm&, const nperm&) = default;
};

// 20 nds
template <class T, class O = nadd<T>>
    requires ncommutative_monoid<O, T>
struct nfenwick {
    int n = 0;
    vector<T> a;
    nfenwick() = default;
    explicit nfenwick(int n) : n(max(0, n)), a(size_t(max(0, n)) + 1, O::id()) { nassert(n >= 0); }
    template <class A> explicit nfenwick(const A& x) : nfenwick(x.len()) {
        nfori(i, v, x)
            a[i + 1] = v;
        nrep(i, n) {
            int j = i + 1 + ((i + 1) & -(i + 1));
            if (j <= n)
                a[j] = O::op(a[j], a[i + 1]);
        }
    }
    int len() const { return n; }
    bool empty() const { return !n; }
    void clear() { fill(a.begin(), a.end(), O::id()); }
    void add(int i, const T& x) {
        nassert(0 <= i && i < n);
        for (++i; i <= n; i += i & -i)
            a[i] = O::op(a[i], x);
    }
    T prefix(int r) const {
        nassert(0 <= r && r <= n);
        T z = O::id();
        for (; r; r -= r & -r)
            z = O::op(a[r], z);
        return z;
    }
    T fold(int l, int r) const
        requires ngroup<O, T>
    {
        nassert(0 <= l && l <= r && r <= n);
        return O::op(O::inv(prefix(l)), prefix(r));
    }
    T get(int i) const
        requires ngroup<O, T>
    {
        nassert(0 <= i && i < n);
        return fold(i, i + 1);
    }
    T get(int i, T d) const
        requires ngroup<O, T>
    {
        return 0 <= i && i < n ? get(i) : move(d);
    }
    int lower(const T& x) const
        requires requires(T a, T b) { a < b; }
    {
        if (!(O::id() < x))
            return 0;
        int i = 0;
        T z = O::id();
        for (int k = bit_floor(unsigned(n)); k; k >>= 1)
            if (i + k <= n) {
                T y = O::op(z, a[i + k]);
                if (y < x)
                    i += k, z = move(y);
            }
        return i == n ? npos : i;
    }
    int lower(const T& x, int d) const
        requires requires(T a, T b) { a < b; }
    {
        int i = lower(x);
        return i == npos ? d : i;
    }
};

template <class T, class O = nadd<T>>
    requires nmonoid<O, T>
struct nseg_iter {
    int n = 0, z = 1;
    vector<T> a;
    nseg_iter() = default;
    explicit nseg_iter(int n) : n(max(0, n)), z(nbitceil(max(1, max(0, n)))), a(size_t(2) * z, O::id()) {
        nassert(n >= 0 && n <= (1 << 30));
    }
    template <class A> explicit nseg_iter(const A& x) : nseg_iter(x.len()) {
        nfori(i, v, x)
            a[z + i] = v;
        for (int i = z - 1; i; --i)
            a[i] = O::op(a[i << 1], a[i << 1 | 1]);
    }
    int len() const { return n; }
    bool empty() const { return !n; }
    void clear() { fill(a.begin(), a.end(), O::id()); }
    void set(int i, T x) {
        nassert(0 <= i && i < n);
        for (a[i += z] = move(x), i >>= 1; i; i >>= 1)
            a[i] = O::op(a[i << 1], a[i << 1 | 1]);
    }
    void apply(int i, const T& x) { set(i, O::op(get(i), x)); }
    const T& get(int i) const {
        nassert(0 <= i && i < n);
        return a[z + i];
    }
    T get(int i, T d) const { return 0 <= i && i < n ? a[z + i] : move(d); }
    T fold(int l, int r) const {
        nassert(0 <= l && l <= r && r <= n);
        T x = O::id(), y = O::id();
        for (l += z, r += z; l < r; l >>= 1, r >>= 1) {
            if (l & 1)
                x = O::op(x, a[l++]);
            if (r & 1)
                y = O::op(a[--r], y);
        }
        return O::op(x, y);
    }
    T fold() const { return a[1]; }
    template <class F> int maxr(int l, F f) const {
        nassert(0 <= l && l <= n && f(O::id()));
        if (l == n)
            return n;
        T s = O::id();
        int k = l + z;
        do {
            while (!(k & 1))
                k >>= 1;
            T v = O::op(s, a[k]);
            if (!f(v)) {
                while (k < z)
                    if (k <<= 1, v = O::op(s, a[k]), f(v))
                        s = move(v), ++k;
                return min(k - z, n);
            }
            s = move(v);
            ++k;
        } while ((k & -k) != k);
        return n;
    }
    template <class F> int minl(int r, F f) const {
        nassert(0 <= r && r <= n && f(O::id()));
        if (!r)
            return 0;
        T s = O::id();
        int k = r + z;
        do {
            --k;
            while (k > 1 && (k & 1))
                k >>= 1;
            T v = O::op(a[k], s);
            if (!f(v)) {
                while (k < z)
                    if (k = k << 1 | 1, v = O::op(a[k], s), f(v))
                        s = move(v), --k;
                return max(0, k + 1 - z);
            }
            s = move(v);
        } while ((k & -k) != k);
        return 0;
    }
};

template <class T, class O = nmin<T>>
    requires nmonoid<O, T>
struct nsparse {
    int n = 0;
    vector<vector<T>> a;
    nsparse() = default;
    template <class A> explicit nsparse(const A& x) : n(x.len()) {
        if (!n)
            return;
        a.emplace_back(n);
        nfori(i, v, x)
            a[0][i] = v;
        for (int k = 1; (1 << k) <= n; ++k) {
            int d = 1 << (k - 1);
            a.emplace_back(n - (d << 1) + 1);
            nrep(i, int(a.back().size()))
                a[k][i] = O::op(a[k - 1][i], a[k - 1][i + d]);
        }
    }
    int len() const { return n; }
    bool empty() const { return !n; }
    T fold(int l, int r) const {
        nassert(0 <= l && l <= r && r <= n);
        if (l == r)
            return O::id();
        int k = bit_width(unsigned(r - l)) - 1;
        return O::op(a[k][l], a[k][r - (1 << k)]);
    }
    T get(int i) const {
        nassert(0 <= i && i < n);
        return a[0][i];
    }
    T get(int i, T d) const { return 0 <= i && i < n ? a[0][i] : move(d); }
};

template <class T, class O = nadd<T>>
    requires nmonoid<O, T>
struct nqueue_agg {
    struct item {
        T x, s;
    };
    vector<item> l, r;
    int len() const { return int(l.size() + r.size()); }
    bool empty() const { return l.empty() && r.empty(); }
    void clear() {
        l.clear();
        r.clear();
    }
    void reserve(int n) {
        l.reserve(n);
        r.reserve(n);
    }
    template <class U> void push(U&& x) {
        T y = forward<U>(x), s = r.empty() ? y : O::op(r.back().s, y);
        r.push_back({move(y), move(s)});
    }
    void moveleft() {
        while (!r.empty()) {
            T x = move(r.back().x);
            r.pop_back();
            T s = l.empty() ? x : O::op(x, l.back().s);
            l.push_back({move(x), move(s)});
        }
    }
    const T& front() {
        nassert(!empty());
        if (l.empty())
            moveleft();
        return l.back().x;
    }
    T front(T d) { return empty() ? move(d) : front(); }
    T pop() {
        nassert(!empty());
        if (l.empty())
            moveleft();
        T x = move(l.back().x);
        l.pop_back();
        return x;
    }
    T pop(T d) { return empty() ? move(d) : pop(); }
    T fold() const {
        return l.empty() ? r.empty() ? O::id() : r.back().s : r.empty() ? l.back().s : O::op(l.back().s, r.back().s);
    }
};

template <class T> struct naddsum_action {
    static constexpr T id() { return T{}; }
    static constexpr T compose(const T& f, const T& g) { return f + g; }
    static constexpr T apply(const T& f, const T& x, int n) { return x + f * T(n); }
};
template <class S, class F, class M, class A>
    requires nmonoid<M, S>
struct nlazyseg {
    int n = 0;
    vector<S> t;
    vector<F> lz;
    nlazyseg() = default;
    explicit nlazyseg(int n)
        : n(n >= 0 && n <= INT_MAX / 4 ? n : 0),
          t(max<size_t>(1, size_t(4) * size_t(n >= 0 && n <= INT_MAX / 4 ? n : 0)), M::id()),
          lz(max<size_t>(1, size_t(4) * size_t(n >= 0 && n <= INT_MAX / 4 ? n : 0)), A::id()) {
        nassert(n >= 0 && n <= INT_MAX / 4);
    }
    template <class X> explicit nlazyseg(const X& x) : nlazyseg(x.len()) {
        if (n)
            build(1, 0, n, x);
    }
    template <class X> void build(int u, int l, int r, const X& x) {
        if (r - l == 1) {
            t[u] = x[l];
            return;
        }
        int m = (l + r) / 2;
        build(u * 2, l, m, x);
        build(u * 2 + 1, m, r, x);
        pull(u);
    }
    int len() const { return n; }
    bool empty() const { return !n; }
    void pull(int u) { t[u] = M::op(t[u * 2], t[u * 2 + 1]); }
    void put(int u, int l, int r, const F& f) {
        t[u] = A::apply(f, t[u], r - l);
        lz[u] = A::compose(f, lz[u]);
    }
    void push(int u, int l, int r) {
        if (r - l < 2)
            return;
        int m = (l + r) / 2;
        put(u * 2, l, m, lz[u]);
        put(u * 2 + 1, m, r, lz[u]);
        lz[u] = A::id();
    }
    void apply0(int u, int l, int r, int ql, int qr, const F& f) {
        if (ql <= l && r <= qr)
            return put(u, l, r, f);
        push(u, l, r);
        int m = (l + r) / 2;
        if (ql < m)
            apply0(u * 2, l, m, ql, qr, f);
        if (m < qr)
            apply0(u * 2 + 1, m, r, ql, qr, f);
        pull(u);
    }
    S fold0(int u, int l, int r, int ql, int qr) {
        if (ql <= l && r <= qr)
            return t[u];
        push(u, l, r);
        int m = (l + r) / 2;
        if (qr <= m)
            return fold0(u * 2, l, m, ql, qr);
        if (m <= ql)
            return fold0(u * 2 + 1, m, r, ql, qr);
        return M::op(fold0(u * 2, l, m, ql, qr), fold0(u * 2 + 1, m, r, ql, qr));
    }
    void set0(int u, int l, int r, int i, S x) {
        if (r - l == 1)
            return t[u] = move(x), void();
        push(u, l, r);
        int m = (l + r) / 2;
        i < m ? set0(u * 2, l, m, i, move(x)) : set0(u * 2 + 1, m, r, i, move(x));
        pull(u);
    }
    void apply(int l, int r, const F& f) {
        nassert(0 <= l && l <= r && r <= n);
        if (l < r)
            apply0(1, 0, n, l, r, f);
    }
    S fold(int l, int r) {
        nassert(0 <= l && l <= r && r <= n);
        return l == r ? M::id() : fold0(1, 0, n, l, r);
    }
    S fold() const { return n ? t[1] : M::id(); }
    void set(int i, S x) {
        nassert(0 <= i && i < n);
        set0(1, 0, n, i, move(x));
    }
    S get(int i) {
        nassert(0 <= i && i < n);
        return fold(i, i + 1);
    }
    S get(int i, S d) { return 0 <= i && i < n ? get(i) : move(d); }
};
template <class T> using nlazy_addsum = nlazyseg<T, T, nadd<T>, naddsum_action<T>>;

struct npart_dense {
  private:
    nvector_stl<int> c;
    int k = 0;

  public:
    npart_dense() = default;
    explicit npart_dense(nvector_stl<int> x) : c(move(x)) {
        nmap_flat<int, int> m;
        nfori(i, x, c) {
            auto p = m.get(x);
            if (!p)
                m.ins(x, k), c[i] = k++;
            else
                c[i] = *p;
        }
    }
    int len() const { return c.len(); }
    int classes() const { return k; }
    int classof(int i, int d = npos) const { return 0 <= i && i < len() ? c[i] : d; }
    bool same(int i, int j) const {
        nassert(0 <= i && i < len() && 0 <= j && j < len());
        return c[i] == c[j];
    }
    nvector_stl<nvector_stl<int>> groups() const {
        nvector_stl<nvector_stl<int>> g(k);
        nfori(i, x, c)
            g[x].push(i);
        return g;
    }
    auto enumerate() const& { return c.enumerate(); }
    auto enumerate() && = delete;
    friend bool operator==(const npart_dense&, const npart_dense&) = default;
};

struct ndsu {
    vector<int> p;
    ndsu() = default;
    explicit ndsu(int n) : p(n, -1) {}
    int len() const { return int(p.size()); }
    int operator()(int x) {
        nassert(0 <= x && x < len());
        return p[x] < 0 ? x : p[x] = (*this)(p[x]);
    }
    int find(int x) { return (*this)(x); }
    int size(int x) { return -p[find(x)]; }
    bool same(int a, int b) { return find(a) == find(b); }
    int merge(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b)
            return a;
        if (p[a] > p[b])
            swap(a, b);
        p[a] += p[b];
        p[b] = a;
        return a;
    }
    bool unite(int a, int b) { return find(a) != find(b) ? merge(a, b), true : false; }
    npart_dense partition() {
        nvector_stl<int> x(len());
        nrep(i, len())
            x[i] = find(i);
        return npart_dense(move(x));
    }
};

struct ndsu_rollback {
    vector<int> p;
    struct change {
        int a, b, sa;
    };
    vector<change> h;
    ndsu_rollback() = default;
    explicit ndsu_rollback(int n) : p(n, -1) {}
    int len() const { return int(p.size()); }
    int find(int x) const {
        nassert(0 <= x && x < len());
        while (p[x] >= 0)
            x = p[x];
        return x;
    }
    int size(int x) const { return -p[find(x)]; }
    bool same(int a, int b) const { return find(a) == find(b); }
    int time() const { return int(h.size()); }
    bool merge(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b)
            return h.push_back({-1, -1, 0}), false;
        if (p[a] > p[b])
            swap(a, b);
        h.push_back({a, b, p[a]});
        p[a] += p[b];
        p[b] = a;
        return true;
    }
    void undo() {
        nassert(!h.empty());
        auto [a, b, sa] = h.back();
        h.pop_back();
        if (a < 0)
            return;
        p[b] = p[a] - sa;
        p[a] = sa;
    }
    void rollback(int t = 0) {
        nassert(0 <= t && t <= time());
        while (time() > t)
            undo();
    }
};

template <class T> struct npotential_dsu {
    vector<int> p;
    vector<T> w;
    npotential_dsu() = default;
    explicit npotential_dsu(int n) : p(n, -1), w(n) {}
    int len() const { return int(p.size()); }
    int find(int x) {
        nassert(0 <= x && x < len());
        if (p[x] < 0)
            return x;
        int q = p[x];
        p[x] = find(q);
        w[x] += w[q];
        return p[x];
    }
    T potential(int x) {
        find(x);
        return w[x];
    }
    int size(int x) { return -p[find(x)]; }
    bool same(int a, int b) { return find(a) == find(b); }
    bool bind(int a, int b, T d) {
        int x = find(a), y = find(b);
        T z = d + w[a] - w[b];
        if (x == y)
            return z == T{};
        if (p[x] > p[y])
            swap(x, y), z = -z;
        p[x] += p[y];
        p[y] = x;
        w[y] = z;
        return true;
    }
    nmaybe<T> diff(int a, int b) {
        if (!same(a, b))
            return {};
        return w[b] - w[a];
    }
    T diff(int a, int b, T d) {
        auto z = diff(a, b);
        return z ? z.val() : move(d);
    }
};

// 30 ninteger
template <integral T> constexpr T nfloor_div(T a, T b) {
    nassert(b);
    if constexpr (is_unsigned_v<T>)
        return a / b;
    else {
        static_assert(sizeof(T) <= 8);
        __int128 q = __int128(a) / b, r = __int128(a) % b;
        q -= r && ((r < 0) != (b < 0));
        nassert(q >= numeric_limits<T>::lowest() && q <= numeric_limits<T>::max());
        return T(q);
    }
}
template <integral T> constexpr T nceil_div(T a, T b) {
    nassert(b);
    if constexpr (is_unsigned_v<T>)
        return a / b + (a % b != 0);
    else {
        static_assert(sizeof(T) <= 8);
        __int128 q = __int128(a) / b, r = __int128(a) % b;
        q += r && ((r < 0) == (b < 0));
        nassert(q >= numeric_limits<T>::lowest() && q <= numeric_limits<T>::max());
        return T(q);
    }
}
template <integral T> constexpr make_unsigned_t<T> ngcd_euclid(T a, T b) {
    using U = make_unsigned_t<T>;
    U x = nmag(a), y = nmag(b);
    while (y) {
        U r = x % y;
        x = y;
        y = r;
    }
    return x;
}
template <integral T> constexpr make_unsigned_t<T> ngcd_binary(T a, T b) {
    using U = make_unsigned_t<T>;
    U x = nmag(a), y = nmag(b);
    if (!x || !y)
        return x | y;
    int z = countr_zero(x | y);
    x >>= countr_zero(x);
    do {
        y >>= countr_zero(y);
        if (x > y)
            swap(x, y);
        y -= x;
    } while (y);
    return x << z;
}
template <integral T> constexpr make_unsigned_t<T> nlcm(T a, T b) {
    using U = make_unsigned_t<T>;
    U x = nmag(a), y = nmag(b);
    if (!x || !y)
        return 0;
    U q = x / ngcd_binary(x, y);
    nassert(y <= numeric_limits<U>::max() / q);
    return q * y;
}
struct nextgcd_result {
    __int128_t g, x, y;
};
constexpr nextgcd_result nextgcd(long long a, long long b) {
    __int128 A = a, B = b, x0 = 1, y0 = 0, x1 = 0, y1 = 1;
    while (B) {
        __int128 q = A / B, t = A % B;
        A = B;
        B = t;
        t = x0 - q * x1;
        x0 = x1;
        x1 = t;
        t = y0 - q * y1;
        y0 = y1;
        y1 = t;
    }
    if (A < 0)
        A = -A, x0 = -x0, y0 = -y0;
    return {A, x0, y0};
}
constexpr uint64_t nmulmod(uint64_t a, uint64_t b, uint64_t m) {
    return uint64_t(__uint128_t(a) * b % m);
}
constexpr uint64_t npowmod(uint64_t a, uint64_t e, uint64_t m) {
    uint64_t r = 1 % m;
    for (; e; e >>= 1, a = nmulmod(a, a, m))
        if (e & 1)
            r = nmulmod(r, a, m);
    return r;
}

template <signed_integral T = long long> struct nfrac {
    using W = nwide_t<T>;
    using U = conditional_t<(sizeof(W) <= 8), uint64_t, __uint128_t>;
    T p = 0, q = 1;
    static U mag(W x) {
        U u = U(x);
        return x < 0 ? U(0) - u : u;
    }
    static U gcd(U a, U b) {
        while (b) {
            U r = a % b;
            a = b;
            b = r;
        }
        return a;
    }
    static T cast(W x) {
        bool ok = x >= numeric_limits<T>::min() && x <= numeric_limits<T>::max();
        nassert(ok);
        return ok ? T(x) : T{};
    }
    void set(W a, W b) {
        if (!b) {
            nassert(false);
            p = 0;
            q = 1;
            return;
        }
        U g = gcd(mag(a), mag(b));
        a /= W(g);
        b /= W(g);
        if (b < 0)
            a = -a, b = -b;
        p = cast(a);
        q = cast(b);
    }
    constexpr nfrac() = default;
    nfrac(T p) : p(p) {}
    nfrac(T p, T q) { set(p, q); }
    nfrac& operator+=(const nfrac& b) {
        U g = gcd(U(q), U(b.q));
        W x = W(q) / W(g), y = W(b.q) / W(g);
        set(W(p) * y + W(b.p) * x, x * b.q);
        return *this;
    }
    nfrac& operator-=(const nfrac& b) { return *this += -b; }
    nfrac& operator*=(const nfrac& b) {
        U x = gcd(mag(p), U(b.q)), y = gcd(mag(b.p), U(q));
        set(W(p) / W(x) * (W(b.p) / W(y)), W(q) / W(y) * (W(b.q) / W(x)));
        return *this;
    }
    nmaybe<nfrac> trydiv(const nfrac& b) const {
        if (!b.p)
            return {};
        nfrac z = *this;
        z *= nfrac(b.q, b.p);
        return z;
    }
    nfrac& operator/=(const nfrac& b) {
        auto z = trydiv(b);
        nassert(z);
        return *this = move(z.val());
    }
    nfrac operator+() const { return *this; }
    nfrac operator-() const { return nfrac(cast(-W(p)), q); }
    friend nfrac operator+(nfrac a, const nfrac& b) { return a += b; }
    friend nfrac operator-(nfrac a, const nfrac& b) { return a -= b; }
    friend nfrac operator*(nfrac a, const nfrac& b) { return a *= b; }
    friend nfrac operator/(nfrac a, const nfrac& b) { return a /= b; }
    friend bool operator==(const nfrac&, const nfrac&) = default;
    friend strong_ordering operator<=>(const nfrac& a, const nfrac& b) {
        W x = W(a.p) * b.q, y = W(b.p) * a.q;
        return x < y ? strong_ordering::less : x > y ? strong_ordering::greater : strong_ordering::equal;
    }
    T floor() const { return nfloor_div(p, q); }
    T ceil() const { return nceil_div(p, q); }
    explicit operator long double() const { return static_cast<long double>(p) / q; }
    friend ostream& operator<<(ostream& o, const nfrac& a) { return o << a.p << '/' << a.q; }
};

struct ncongruence {
    long long a = 0, m = 1;
    ncongruence() = default;
    ncongruence(long long a, long long m) : m(m) {
        nassert(m > 0);
        this->a = a % m;
        if (this->a < 0)
            this->a += m;
    }
    bool has(long long x) const { return (__int128(x) - a) % m == 0; }
    nmaybe<long long> at(long long k) const {
        __int128 z = __int128(a) + __int128(k) * m;
        return z < numeric_limits<long long>::min() || z > numeric_limits<long long>::max()
                   ? nmaybe<long long>{}
                   : nmaybe<long long>(static_cast<long long>(z));
    }
    long long at(long long k, long long d) const {
        auto z = at(k);
        return z ? z.val() : move(d);
    }
    long long operator()(long long k) const {
        auto z = at(k);
        nassert(z);
        return z.val();
    }
    friend bool operator==(const ncongruence&, const ncongruence&) = default;
};
inline nmaybe<ncongruence> ncrt(ncongruence x, ncongruence y) {
    auto [g, p, q] = nextgcd(x.m, y.m);
    (void)q;
    __int128 d = __int128(y.a) - x.a;
    if (d % g)
        return {};
    __int128 l = __int128(x.m / g) * y.m;
    if (l > numeric_limits<long long>::max())
        return {};
    __int128 mod = y.m / g, k = (d / g) * p % mod, a = __int128(x.a) + __int128(x.m) * k;
    long long m = static_cast<long long>(l), z = static_cast<long long>(a % l);
    if (z < 0)
        z += m;
    return ncongruence(z, m);
}
inline ncongruence ncrt(ncongruence x, ncongruence y, ncongruence d) {
    auto z = ncrt(x, y);
    return z ? z.val() : move(d);
}

#define ni_mod_ops(S)                                                                                                  \
    constexpr S& operator+=(S b) {                                                                                     \
        x += b.x;                                                                                                      \
        if (x >= mod())                                                                                                \
            x -= mod();                                                                                                \
        return *this;                                                                                                  \
    }                                                                                                                  \
    constexpr S& operator-=(S b) {                                                                                     \
        x = x >= b.x ? x - b.x : x + mod() - b.x;                                                                      \
        return *this;                                                                                                  \
    }                                                                                                                  \
    constexpr S& operator*=(S b) {                                                                                     \
        x = uint64_t(__uint128_t(x) * b.x % mod());                                                                    \
        return *this;                                                                                                  \
    }                                                                                                                  \
    constexpr nmaybe<S> tryinv() const {                                                                               \
        auto z = nextgcd((long long)x, (long long)mod());                                                              \
        return z.g == 1 ? nmaybe<S>(S(z.x)) : nmaybe<S>{};                                                             \
    }                                                                                                                  \
    constexpr S inv() const {                                                                                          \
        auto z = tryinv();                                                                                             \
        nassert(z);                                                                                                    \
        return z.val();                                                                                                \
    }                                                                                                                  \
    constexpr S inv(S d) const {                                                                                       \
        auto z = tryinv();                                                                                             \
        return z ? z.val() : move(d);                                                                                  \
    }                                                                                                                  \
    constexpr S& operator/=(S b) {                                                                                     \
        return *this *= b.inv();                                                                                       \
    }                                                                                                                  \
    constexpr S operator+() const {                                                                                    \
        return *this;                                                                                                  \
    }                                                                                                                  \
    constexpr S operator-() const {                                                                                    \
        return x ? raw(mod() - x) : *this;                                                                             \
    }                                                                                                                  \
    friend constexpr S operator+(S a, S b) {                                                                           \
        return a += b;                                                                                                 \
    }                                                                                                                  \
    friend constexpr S operator-(S a, S b) {                                                                           \
        return a -= b;                                                                                                 \
    }                                                                                                                  \
    friend constexpr S operator*(S a, S b) {                                                                           \
        return a *= b;                                                                                                 \
    }                                                                                                                  \
    friend constexpr S operator/(S a, S b) {                                                                           \
        return a /= b;                                                                                                 \
    }                                                                                                                  \
    constexpr S& operator++() {                                                                                        \
        return *this += 1;                                                                                             \
    }                                                                                                                  \
    constexpr S operator++(int) {                                                                                      \
        S a = *this;                                                                                                   \
        ++*this;                                                                                                       \
        return a;                                                                                                      \
    }                                                                                                                  \
    constexpr S& operator--() {                                                                                        \
        return *this -= 1;                                                                                             \
    }                                                                                                                  \
    constexpr S operator--(int) {                                                                                      \
        S a = *this;                                                                                                   \
        --*this;                                                                                                       \
        return a;                                                                                                      \
    }                                                                                                                  \
    constexpr explicit operator uint64_t() const {                                                                     \
        return x;                                                                                                      \
    }                                                                                                                  \
    friend constexpr auto operator<=>(const S&, const S&) = default;                                                   \
    friend ostream& operator<<(ostream& o, S a) {                                                                      \
        return o << a.x;                                                                                               \
    }                                                                                                                  \
    friend istream& operator>>(istream& i, S& a) {                                                                     \
        long long x;                                                                                                   \
        i >> x;                                                                                                        \
        a = S(x);                                                                                                      \
        return i;                                                                                                      \
    }

template <uint64_t M> struct nmod_static {
    static_assert(0 < M && M <= uint64_t(numeric_limits<long long>::max()));
    uint64_t x = 0;
    static constexpr uint64_t mod() { return M; }
    static constexpr nmod_static raw(uint64_t x) {
        nmod_static a;
        a.x = x;
        return a;
    }
    constexpr nmod_static() = default;
    template <integral I> constexpr nmod_static(I v) {
        if constexpr (is_signed_v<I>) {
            __int128 z = __int128(v) % M;
            if (z < 0)
                z += M;
            x = uint64_t(z);
        } else
            x = uint64_t(__uint128_t(v) % M);
    }
    ni_mod_ops(nmod_static)
};
template <int Tag = 0> struct nmod_dynamic {
    static inline uint64_t M = 1;
    uint64_t x = 0;
    static uint64_t mod() { return M; }
    static void setmod(uint64_t m) {
        nassert(0 < m && m <= uint64_t(numeric_limits<long long>::max()));
        M = m;
    }
    static constexpr nmod_dynamic raw(uint64_t x) {
        nmod_dynamic a;
        a.x = x;
        return a;
    }
    nmod_dynamic() = default;
    template <integral I> nmod_dynamic(I v) {
        uint64_t m = mod();
        if constexpr (is_signed_v<I>) {
            __int128 z = __int128(v) % m;
            if (z < 0)
                z += m;
            x = uint64_t(z);
        } else
            x = uint64_t(__uint128_t(v) % m);
    }
    ni_mod_ops(nmod_dynamic)
};
#undef ni_mod_ops

inline bool nisprime_trial(uint64_t n) {
    if (n < 2)
        return false;
    for (uint64_t p = 2; p <= n / p; ++p)
        if (n % p == 0)
            return false;
    return true;
}
inline bool nisprime_miller(uint64_t n) {
    if (n < 2)
        return false;
    for (uint64_t p : {2ULL, 3ULL, 5ULL, 7ULL, 11ULL, 13ULL, 17ULL, 19ULL, 23ULL, 29ULL, 31ULL, 37ULL})
        if (n % p == 0)
            return n == p;
    uint64_t d = n - 1, s = countr_zero(d);
    d >>= s;
    for (uint64_t a : {2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL, 9780504ULL, 1795265022ULL}) {
        if (a % n == 0)
            continue;
        uint64_t x = npowmod(a % n, d, n);
        if (x == 1 || x == n - 1)
            continue;
        bool ok = false;
        for (uint64_t r = 1; r < s; ++r)
            if ((x = nmulmod(x, x, n)) == n - 1) {
                ok = true;
                break;
            }
        if (!ok)
            return false;
    }
    return true;
}
inline uint64_t npollard(uint64_t n) {
    if (!(n & 1))
        return 2;
    if (n % 3 == 0)
        return 3;
    for (;;) {
        uint64_t c = nrng_global(n - 1) + 1, x = nrng_global(n - 2) + 2, y = x, d = 1;
        auto f = [&](uint64_t v) { return uint64_t((__uint128_t(nmulmod(v, v, n)) + c) % n); };
        while (d == 1)
            x = f(x), y = f(f(y)), d = gcd(x > y ? x - y : y - x, n);
        if (d != n)
            return d;
    }
}
inline vector<uint64_t> nfactor_rho(uint64_t n) {
    vector<uint64_t> a, st;
    if (n > 1)
        st.push_back(n);
    while (!st.empty()) {
        uint64_t x = st.back();
        st.pop_back();
        if (nisprime_miller(x))
            a.push_back(x);
        else {
            uint64_t d = npollard(x);
            st.push_back(d);
            st.push_back(x / d);
        }
    }
    sort(a.begin(), a.end());
    return a;
}

struct nprime_table {
    int n = 0;
    vector<int> p, lp, phi, mu;
    nprime_table() = default;
    explicit nprime_table(int n)
        : n(n < 0 ? 0 : n), lp(size_t(n < 0 ? 0 : n) + 1), phi(size_t(n < 0 ? 0 : n) + 1),
          mu(size_t(n < 0 ? 0 : n) + 1) {
        nassert(n >= 0);
        if (this->n >= 1)
            phi[1] = mu[1] = 1;
        for (int i = 2; i <= this->n; ++i) {
            if (!lp[i])
                lp[i] = i, p.push_back(i), phi[i] = i - 1, mu[i] = -1;
            for (int q : p) {
                if (q > lp[i] || 1LL * i * q > this->n)
                    break;
                lp[i * q] = q;
                if (q == lp[i])
                    phi[i * q] = phi[i] * q, mu[i * q] = 0;
                else
                    phi[i * q] = phi[i] * (q - 1), mu[i * q] = -mu[i];
            }
        }
    }
    bool isprime(int x) const { return 2 <= x && x <= n && lp[x] == x; }
    vector<pair<int, int>> factor(int x) const {
        nassert(0 < x && x <= n);
        vector<pair<int, int>> z;
        while (x > 1) {
            int q = lp[x], c = 0;
            do
                x /= q, ++c;
            while (x > 1 && lp[x] == q);
            z.push_back({q, c});
        }
        return z;
    }
    vector<int> divisors(int x) const {
        vector<int> d{1};
        for (auto [q, c] : factor(x)) {
            int z = int(d.size()), w = 1;
            for (int j = 1; j <= c; ++j) {
                w *= q;
                nrep(i, z)
                    d.push_back(d[i] * w);
            }
        }
        sort(d.begin(), d.end());
        return d;
    }
};

// 31 ncomb
template <class T> struct ncomb {
    vector<T> f{T{1}}, g{T{1}};
    ncomb() = default;
    explicit ncomb(int n) { build(n); }
    int len() const { return int(f.size()) - 1; }
    void build(int n) {
        nassert(n >= 0);
        if (n < 0)
            return;
        f.resize(size_t(n) + 1);
        g.resize(size_t(n) + 1);
        f[0] = 1;
        for (int i = 1; i <= n; ++i)
            f[i] = f[i - 1] * T(i);
        g[n] = T(1) / f[n];
        for (int i = n; i; --i)
            g[i - 1] = g[i] * T(i);
    }
    T C(long long n, long long k, T d = T{}) const {
        return 0 <= k && k <= n && n <= len() ? f[n] * g[k] * g[n - k] : move(d);
    }
    T P(long long n, long long k, T d = T{}) const {
        return 0 <= k && k <= n && n <= len() ? f[n] * g[n - k] : move(d);
    }
    T H(long long n, long long k, T d = T{}) const {
        return n == 0 && k == 0 ? T{1} : n <= 0 || k < 0 ? move(d) : C(n + k - 1, k, move(d));
    }
    T operator()(long long n, long long k, T d = T{}) const { return C(n, k, move(d)); }
};
template <class A> void nzeta_subset(A& a, bool inv = false) {
    int n = a.len();
    nassert(n >= 0 && !(n & (n - 1)));
    if (!n)
        return;
    for (int b = 1; b < n; b <<= 1)
        for (int m = 0; m < n; m += b * 2)
            nrep(i, b)
                inv ? a[m + b + i] -= a[m + i] : a[m + b + i] += a[m + i];
}
template <class A> void nzeta_superset(A& a, bool inv = false) {
    int n = a.len();
    nassert(n >= 0 && !(n & (n - 1)));
    if (!n)
        return;
    for (int b = 1; b < n; b <<= 1)
        for (int m = 0; m < n; m += b * 2)
            nrep(i, b)
                inv ? a[m + i] -= a[m + b + i] : a[m + i] += a[m + b + i];
}

// 32 nlinear
template <class T, class Add = nadd<T>, class Mul = nmul<T>>
    requires nmonoid<Add, T> && nmonoid<Mul, T>
struct nmat {
    int h = 0, w = 0;
    nvector_stl<T> a;
    nmat() = default;
    nmat(int h, int w, T x = Add::id())
        : h(h >= 0 && w >= 0 && 1LL * h * w <= INT_MAX ? h : 0), w(h >= 0 && w >= 0 && 1LL * h * w <= INT_MAX ? w : 0),
          a(h >= 0 && w >= 0 && 1LL * h * w <= INT_MAX ? int(1LL * h * w) : 0, move(x)) {
        nassert(h >= 0 && w >= 0 && 1LL * h * w <= INT_MAX);
    }
    static nmat eye(int n) {
        nmat a(n, n);
        nrep(i, a.h)
            a(i, i) = Mul::id();
        return a;
    }
    int rows() const { return h; }
    int cols() const { return w; }
    int len() const { return h * w; }
    bool empty() const { return !h || !w; }
    T& operator()(int i, int j) {
        nassert(0 <= i && i < h && 0 <= j && j < w);
        return a[i * w + j];
    }
    const T& operator()(int i, int j) const {
        nassert(0 <= i && i < h && 0 <= j && j < w);
        return a[i * w + j];
    }
    T get(int i, int j, T d = Add::id()) const { return 0 <= i && i < h && 0 <= j && j < w ? a[i * w + j] : move(d); }
    nmat& operator+=(const nmat& b) {
        nassert(h == b.h && w == b.w);
        nrep(i, len())
            a[i] = Add::op(a[i], b.a[i]);
        return *this;
    }
    friend nmat operator+(nmat a, const nmat& b) { return a += b; }
    friend nmat operator*(const nmat& x, const nmat& y) {
        nassert(x.w == y.h);
        nmat z(x.h, y.w);
        nrep(i, x.h)
            nrep(k, x.w) {
                T v = x(i, k);
                if constexpr (requires { v == Add::id(); })
                    if (v == Add::id())
                        continue;
                nrep(j, y.w)
                    z(i, j) = Add::op(z(i, j), Mul::op(v, y(k, j)));
            }
        return z;
    }
    nmat& operator*=(const nmat& b) { return *this = *this * b; }
    nmat pow(long long e) const {
        nassert(h == w && e >= 0);
        nmat x = *this, r = eye(h);
        for (; e; e >>= 1, x *= x)
            if (e & 1)
                r *= x;
        return r;
    }
    nmat trans() const {
        nmat z(w, h);
        nrep(i, h)
            nrep(j, w)
                z(j, i) = (*this)(i, j);
        return z;
    }
    friend bool operator==(const nmat&, const nmat&) = default;
};

template <class T> struct nlinear_solution {
    bool consistent = true;
    int rank = 0;
    nvector_stl<T> one;
    nvector_stl<nvector_stl<T>> basis;
};
template <class T> nlinear_solution<T> ngauss(nmat<T> a, nvector_stl<T> b) {
    int m = a.rows(), n = a.cols(), r = 0;
    nassert(b.len() == m);
    vector<int> w(n, npos);
    for (int c = 0; c < n && r < m; ++c) {
        int s = r;
        while (s < m && a(s, c) == T{})
            ++s;
        if (s == m)
            continue;
        nrep(j, n)
            swap(a(r, j), a(s, j));
        swap(b[r], b[s]);
        T q = T{1} / a(r, c);
        for (int j = c; j < n; ++j)
            a(r, j) *= q;
        b[r] *= q;
        nrep(i, m)
            if (i != r && a(i, c) != T{}) {
                T z = a(i, c);
                for (int j = c; j < n; ++j)
                    a(i, j) -= z * a(r, j);
                b[i] -= z * b[r];
            }
        w[c] = r++;
    }
    nrep(i, m) {
        bool z = true;
        nrep(j, n)
            z &= a(i, j) == T{};
        if (z && b[i] != T{})
            return {false, r, {}, {}};
    }
    nlinear_solution<T> z;
    z.rank = r;
    z.one = nvector_stl<T>(n);
    nrep(c, n)
        if (w[c] != npos)
            z.one[c] = b[w[c]];
    nrep(f, n)
        if (w[f] == npos) {
            nvector_stl<T> v(n);
            v[f] = T{1};
            nrep(c, n)
                if (w[c] != npos)
                    v[c] = -a(w[c], f);
            z.basis.push(move(v));
        }
    return z;
}
template <class T> T ndet(nmat<T> a) {
    nassert(a.rows() == a.cols());
    int n = a.rows();
    T z = 1;
    nrep(c, n) {
        int s = c;
        while (s < n && a(s, c) == T{})
            ++s;
        if (s == n)
            return T{};
        if (s != c) {
            nrep(j, n)
                swap(a(c, j), a(s, j));
            z = -z;
        }
        T p = a(c, c);
        z *= p;
        T q = T{1} / p;
        for (int j = c; j < n; ++j)
            a(c, j) *= q;
        for (int i = c + 1; i < n; ++i)
            if (a(i, c) != T{}) {
                T x = a(i, c);
                for (int j = c; j < n; ++j)
                    a(i, j) -= x * a(c, j);
            }
    }
    return z;
}
template <class T> nmaybe<nmat<T>> ninverse(nmat<T> a) {
    nassert(a.rows() == a.cols());
    int n = a.rows();
    nmat<T> b = nmat<T>::eye(n);
    nrep(c, n) {
        int s = c;
        while (s < n && a(s, c) == T{})
            ++s;
        if (s == n)
            return {};
        nrep(j, n)
            swap(a(c, j), a(s, j)), swap(b(c, j), b(s, j));
        T q = T{1} / a(c, c);
        nrep(j, n)
            a(c, j) *= q, b(c, j) *= q;
        nrep(i, n)
            if (i != c && a(i, c) != T{}) {
                T x = a(i, c);
                nrep(j, n)
                    a(i, j) -= x * a(c, j), b(i, j) -= x * b(c, j);
            }
    }
    return b;
}
template <class T> nmat<T> ninverse(nmat<T> a, nmat<T> d) {
    auto z = ninverse(move(a));
    return z ? move(z.val()) : move(d);
}

template <class T> nvector_stl<T> nberlekamp(const nvector_stl<T>& s) {
    vector<T> C{T{1}}, B{T{1}};
    int L = 0, m = 1;
    T b = 1;
    nrep(n, s.len()) {
        T d = s[n];
        for (int i = 1; i <= L; ++i)
            d += C[i] * s[n - i];
        if (d == T{}) {
            ++m;
            continue;
        }
        vector<T> A = C;
        T q = d / b;
        if (int(C.size()) < int(B.size()) + m)
            C.resize(B.size() + m);
        nrep(i, int(B.size()))
            C[i + m] -= q * B[i];
        if (2 * L <= n)
            L = n + 1 - L, B = move(A), b = d, m = 1;
        else
            ++m;
    }
    nvector_stl<T> z(L);
    nrep(i, L)
        z[i] = -C[i + 1];
    return z;
}
template <class T> nmaybe<T> nrec_nth(const nvector_stl<T>& a, const nvector_stl<T>& c, uint64_t k) {
    int n = c.len();
    if (k < uint64_t(a.len()))
        return a[int(k)];
    if (!n || a.len() < n)
        return {};
    auto mul = [&](const vector<T>& x, const vector<T>& y) {
        vector<T> z(n * 2 - 1);
        nrep(i, n)
            nrep(j, n)
                z[i + j] += x[i] * y[j];
        for (int i = n * 2 - 1; i-- > n;)
            nrep(j, n)
                z[i - j - 1] += z[i] * c[j];
        z.resize(n);
        return z;
    };
    vector<T> x(n), y(n);
    x[0] = 1;
    n == 1 ? y[0] = c[0] : y[1] = 1;
    for (; k; k >>= 1, y = mul(y, y))
        if (k & 1)
            x = mul(x, y);
    T z{};
    nrep(i, n)
        z += x[i] * a[i];
    return z;
}
template <class T> T nrec_nth(const nvector_stl<T>& a, const nvector_stl<T>& c, uint64_t k, T d) {
    auto z = nrec_nth(a, c, k);
    return z ? z.val() : move(d);
}

template <unsigned_integral T = uint64_t> struct nxorbasis {
    static constexpr int B = numeric_limits<T>::digits;
    array<T, B> a{};
    int n = 0;
    int len() const { return n; }
    bool ins(T x) {
        for (int i = B - 1; i >= 0; --i)
            if (x >> i & 1) {
                if (a[i])
                    x ^= a[i];
                else {
                    a[i] = x;
                    ++n;
                    return true;
                }
            }
        return false;
    }
    bool has(T x) const {
        for (int i = B - 1; i >= 0; --i)
            if (x >> i & 1)
                x ^= a[i];
        return !x;
    }
    T max(T x = 0) const {
        for (int i = B - 1; i >= 0; --i)
            nchmax(x, T(x ^ a[i]));
        return x;
    }
    T min_nonzero(T d = 0) const {
        for (T x : a)
            if (x)
                return x;
        return d;
    }
};

// 33 npoly
template <class T> struct nntt_info {
    static constexpr bool ok = false;
};
template <> struct nntt_info<nmod_static<998244353>> {
    static constexpr bool ok = true;
    static constexpr uint64_t root = 3;
};
template <> struct nntt_info<nmod_static<1004535809>> {
    static constexpr bool ok = true;
    static constexpr uint64_t root = 3;
};
template <> struct nntt_info<nmod_static<469762049>> {
    static constexpr bool ok = true;
    static constexpr uint64_t root = 3;
};

template <class T>
    requires(nntt_info<T>::ok)
void nntt(vector<T>& a, bool inv = false) {
    int n = int(a.size());
    nassert(n && !(n & (n - 1)) && (T::mod() - 1) % uint64_t(n) == 0);
    for (int i = 1, j = 0; i < n; ++i) {
        int b = n >> 1;
        for (; j & b; b >>= 1)
            j ^= b;
        j ^= b;
        if (i < j)
            swap(a[i], a[j]);
    }
    for (int l = 2; l <= n; l <<= 1) {
        T w = npow(T(nntt_info<T>::root), static_cast<long long>((T::mod() - 1) / uint64_t(l)));
        if (inv)
            w = w.inv();
        for (int i = 0; i < n; i += l) {
            T x = 1;
            nrep(j, l / 2) {
                T u = a[i + j], v = a[i + j + l / 2] * x;
                a[i + j] = u + v;
                a[i + j + l / 2] = u - v;
                x *= w;
            }
        }
    }
    if (inv) {
        T z = T(n).inv();
        for (T& x : a)
            x *= z;
    }
}
template <class T> nvector_stl<T> nconv_naive(const nvector_stl<T>& a, const nvector_stl<T>& b) {
    if (a.empty() || b.empty())
        return {};
    long long q = 1LL * a.len() + b.len() - 1;
    nassert(q <= INT_MAX);
    if (q > INT_MAX)
        return {};
    nvector_stl<T> c(int(q), T{});
    nfori(i, x, a)
        nfori(j, y, b)
            c[i + j] += x * y;
    return c;
}
template <class T>
    requires(nntt_info<T>::ok)
nvector_stl<T> nconv_ntt(const nvector_stl<T>& a, const nvector_stl<T>& b) {
    if (a.empty() || b.empty())
        return {};
    long long q = 1LL * a.len() + b.len() - 1;
    nassert(q > 0 && q <= (1 << 30));
    if (q <= 0 || q > (1 << 30))
        return {};
    int z = nbitceil(int(q));
    nassert((T::mod() - 1) % uint64_t(z) == 0);
    vector<T> x(z), y(z);
    nrep(i, a.len())
        x[i] = a[i];
    nrep(i, b.len())
        y[i] = b[i];
    nntt(x);
    nntt(y);
    nrep(i, z)
        x[i] *= y[i];
    nntt(x, true);
    x.resize(size_t(q));
    nvector_stl<T> r;
    r.a = move(x);
    return r;
}
template <class T> nvector_stl<T> nconv_auto(const nvector_stl<T>& a, const nvector_stl<T>& b) {
    if (1LL * a.len() * b.len() <= 4096)
        return nconv_naive(a, b);
    if constexpr (nntt_info<T>::ok)
        return nconv_ntt(a, b);
    else
        return nconv_naive(a, b);
}

template <class T> struct npoly {
    nvector_stl<T> a;
    npoly() = default;
    npoly(initializer_list<T> x) : a(x) { norm(); }
    explicit npoly(nvector_stl<T> x) : a(move(x)) { norm(); }
    int len() const { return a.len(); }
    int deg() const { return len() - 1; }
    bool empty() const { return a.empty(); }
    void norm() {
        while (!a.empty() && a.back() == T{})
            a.pop();
    }
    T operator[](int i) const { return 0 <= i && i < len() ? a[i] : T{}; }
    T& at(int i) {
        nassert(0 <= i && i < len());
        return a[i];
    }
    T operator()(T x) const {
        T y{};
        for (int i = len() - 1; i >= 0; --i)
            y = y * x + a[i];
        return y;
    }
    npoly& operator+=(const npoly& b) {
        if (len() < b.len())
            a.resize(b.len(), T{});
        nrep(i, b.len())
            a[i] += b.a[i];
        norm();
        return *this;
    }
    npoly& operator-=(const npoly& b) {
        if (len() < b.len())
            a.resize(b.len(), T{});
        nrep(i, b.len())
            a[i] -= b.a[i];
        norm();
        return *this;
    }
    npoly& operator*=(const npoly& b) {
        a = nconv_auto(a, b.a);
        norm();
        return *this;
    }
    friend npoly operator+(npoly a, const npoly& b) { return a += b; }
    friend npoly operator-(npoly a, const npoly& b) { return a -= b; }
    friend npoly operator*(npoly a, const npoly& b) { return a *= b; }
    friend bool operator==(const npoly&, const npoly&) = default;
    npoly deriv() const {
        if (len() < 2)
            return {};
        nvector_stl<T> x(len() - 1);
        for (int i = 1; i < len(); ++i)
            x[i - 1] = a[i] * T(i);
        return npoly(move(x));
    }
    npoly integral() const {
        nvector_stl<T> x(len() + 1, T{});
        nrep(i, len())
            x[i + 1] = a[i] / T(i + 1);
        return npoly(move(x));
    }
    npoly cut(int n) const {
        nassert(n >= 0);
        if (n < 0)
            return {};
        nvector_stl<T> x(min(n, len()));
        nrep(i, x.len())
            x[i] = a[i];
        return npoly(move(x));
    }
    npoly inv(int n) const {
        nassert(n >= 0);
        if (!n)
            return {};
        nassert(!empty());
        npoly r{T{1} / a[0]};
        for (int m = 1; m < n;) {
            int k = min(n, m * 2);
            npoly q = (cut(k) * r).cut(k);
            nvector_stl<T> v(k);
            v[0] = T(2) - q[0];
            for (int i = 1; i < k; ++i)
                v[i] = -q[i];
            r = (r * npoly(move(v))).cut(k);
            m = k;
        }
        return r.cut(n);
    }
    npoly log(int n) const {
        nassert(n >= 0);
        if (!n)
            return {};
        nassert((*this)[0] == T{1});
        return (deriv() * inv(n)).cut(n - 1).integral().cut(n);
    }
    npoly exp(int n) const {
        nassert(n >= 0);
        if (!n)
            return {};
        nassert((*this)[0] == T{});
        npoly r{T{1}};
        for (int m = 1; m < n;) {
            int k = min(n, m * 2);
            npoly q = cut(k) - r.log(k);
            if (q.empty())
                q.a.resize(1);
            q.a[0] += T{1};
            r = (r * q).cut(k);
            m = k;
        }
        return r.cut(n);
    }
};

// 40 ngraph
template <class T> constexpr T ncapadd(T a, T b, T inf = ninf<T>) {
    if constexpr (integral<T> && sizeof(T) <= 8) {
        using W = conditional_t<is_signed_v<T>, __int128_t, __uint128_t>;
        W z = W(a) + W(b);
        if (z > W(inf))
            return inf;
        if constexpr (is_signed_v<T>)
            if (z < W(nninf<T>))
                return nninf<T>;
        return T(z);
    } else
        return a + b;
}
template <class W> struct nedge {
    int from, to, id;
    W w;
};
template <class G> struct ngraph_arcs_view {
    G* g;
    using base_cursor = decltype(nenumerate(declval<G&>()[0]));
    struct cursor {
        G* g;
        int u = 0, k = 0;
        optional<base_cursor> e;
        explicit cursor(G* x) : g(x) { seek(); }
        void seek() {
            while (u < g->len()) {
                if (!e)
                    e.emplace(nenumerate((*g)[u]));
                if (e->ok())
                    return;
                ++u;
                e.reset();
            }
        }
        bool ok() const { return u < g->len(); }
        decltype(auto) val() const { return e->val(); }
        int idx() const { return k; }
        void next() {
            e->next();
            ++k;
            seek();
        }
    };
    auto enumerate() const { return cursor{g}; }
    int len() const {
        if constexpr (requires { g->edges(); })
            return int(g->edges());
        else {
            int z = 0;
            auto e = enumerate();
            while (e.ok())
                ++z, e.next();
            return z;
        }
    }
};
template <class G> auto nvertices(const G& g) { return nrange(g.len()); }
template <class G> auto narcs(G& g) { return ngraph_arcs_view<G>{&g}; }
template <class G> auto narcs(const G&&) = delete;

template <class G, class P> struct ngraph_where_view {
    const G* g;
    P p;
    using base_cursor = decltype(nenumerate(declval<const G&>()[0]));
    struct cursor {
        base_cursor e;
        const P* p;
        int k = 0;
        cursor(base_cursor x, const P* q) : e(move(x)), p(q) { skip(); }
        void skip() {
            while (e.ok() && !(*p)(e.val()))
                e.next();
        }
        bool ok() const { return e.ok(); }
        decltype(auto) val() const { return e.val(); }
        int idx() const { return k; }
        void next() {
            e.next();
            ++k;
            skip();
        }
    };
    struct view {
        const ngraph_where_view* a;
        int u;
        auto enumerate() const { return cursor{nenumerate((*a->g)[u]), &a->p}; }
        int len() const {
            int z = 0;
            auto e = enumerate();
            while (e.ok())
                ++z, e.next();
            return z;
        }
    };
    int len() const { return g->len(); }
    bool empty() const { return !len(); }
    view operator[](int u) const& {
        nassert(0 <= u && u < len());
        return {this, u};
    }
    view operator[](int) const&& = delete;
    view from(int u) const& { return (*this)[u]; }
    view from(int) const&& = delete;
    int edges() const {
        int z = 0;
        nrep(u, len())
            z += (*this)[u].len();
        return z;
    }
    auto vertices() const { return nvertices(*this); }
    auto arcs() const& { return narcs(*this); }
    auto arcs() const&& = delete;
};
template <class G, class P> auto ngraph_where(const G& g, P p) {
    return ngraph_where_view<G, P>{&g, move(p)};
}
template <class G, class P> auto ngraph_where(const G&&, P) = delete;

template <class W = int> struct ngraph_forward {
    using edge = nedge<W>;
  private:
    int n = 0;
    vector<int> h, to, nx;
    vector<W> w;

  public:
    ngraph_forward() = default;
    explicit ngraph_forward(int n, int m = 0) : n(max(0, n)), h(size_t(max(0, n)), -1) {
        nassert(n >= 0);
        reserve(m);
    }
    int len() const { return n; }
    int edges() const { return int(to.size()); }
    bool empty() const { return !n; }
    void reserve(int m) {
        nassert(m >= 0);
        if (m < 0)
            return;
        to.reserve(size_t(m));
        nx.reserve(size_t(m));
        w.reserve(size_t(m));
    }
    void clear_edges() {
        fill(h.begin(), h.end(), -1);
        to.clear();
        nx.clear();
        w.clear();
    }
    int add(int u, int v, W z = W{1}) {
        nassert(0 <= u && u < n && 0 <= v && v < n);
        int e = edges();
        to.push_back(v);
        w.push_back(move(z));
        nx.push_back(h[u]);
        h[u] = e;
        return e;
    }
    pair<int, int> add2(int u, int v, W z = W{1}) {
        int a = add(u, v, z), b = add(v, u, move(z));
        return {a, b};
    }
    template <bool C> struct cursor_t {
        using graph_type = conditional_t<C, const ngraph_forward, ngraph_forward>;
        using weight_type = conditional_t<C, const W&, W&>;
        graph_type* g;
        int u, e, k = 0;
        bool ok() const { return e != npos; }
        nedge<weight_type> val() const { return {u, g->to[e], e, g->w[e]}; }
        int idx() const { return k; }
        void next() {
            e = g->nx[e];
            ++k;
        }
    };
    template <bool C> struct view_t {
        using graph_type = conditional_t<C, const ngraph_forward, ngraph_forward>;
        graph_type* g;
        int u;
        auto enumerate() const { return cursor_t<C>{g, u, g->h[u]}; }
        int len() const {
            int z = 0;
            for (int e = g->h[u]; e != npos; e = g->nx[e])
                ++z;
            return z;
        }
    };
    using view = view_t<false>;
    using const_view = view_t<true>;
    view operator[](int u) & {
        nassert(0 <= u && u < n);
        return {this, u};
    }
    const_view operator[](int u) const& {
        nassert(0 <= u && u < n);
        return {this, u};
    }
    view operator[](int) && = delete;
    const_view operator[](int) const&& = delete;
    view from(int u) & { return (*this)[u]; }
    const_view from(int u) const& { return (*this)[u]; }
    view from(int) && = delete;
    const_view from(int) const&& = delete;
    int degree(int u) const { return from(u).len(); }
    int find(int u, int v, int d = npos) const {
        nassert(0 <= u && u < n && 0 <= v && v < n);
        nfor(e, (*this)[u])
            if (e.to == v)
                return e.id;
        return d;
    }
    bool has(int u, int v) const { return find(u, v) != npos; }
    W* weight(int e) { return 0 <= e && e < edges() ? &w[e] : nullptr; }
    const W* weight(int e) const { return 0 <= e && e < edges() ? &w[e] : nullptr; }
    W weight(int e, W d) const { return 0 <= e && e < edges() ? w[e] : move(d); }
    bool set(int e, W z) {
        if (e < 0 || e >= edges())
            return false;
        w[e] = move(z);
        return true;
    }
    auto vertices() const { return nvertices(*this); }
    auto arcs() & { return narcs(*this); }
    auto arcs() const& { return narcs(*this); }
    auto arcs() && = delete;
    auto arcs() const&& = delete;
    ngraph_forward reverse() const {
        ngraph_forward z(n, edges());
        nrep(u, n)
            nfor(e, (*this)[u])
                z.add(e.to, u, e.w);
        return z;
    }
};

namespace ni {
template <class G> using ngraph_edge_t = decltype(nenumerate(declval<const G&>()[0]).val());
template <class G> using ngraph_weight_t = remove_cvref_t<decltype(declval<ngraph_edge_t<G>>().w)>;
} // namespace ni

template <class W = int> struct ngraph_csr {
    using edge = nedge<W>;
  private:
    int n = 0;
    vector<int> h, to;
    vector<W> w;

  public:
    ngraph_csr() : h(1) {}
    explicit ngraph_csr(int z) : n(max(0, z)), h(size_t(max(0, z)) + 1) { nassert(z >= 0); }
    template <class G> explicit ngraph_csr(const G& g) { build(g); }
    template <class G> void build(const G& g) {
        n = g.len();
        h.assign(size_t(n) + 1, 0);
        nrep(u, n) {
            nfor(e, g[u])
                (void)e, ++h[u + 1];
            h[u + 1] += h[u];
        }
        to.clear();
        w.clear();
        to.reserve(h[n]);
        w.reserve(h[n]);
        nrep(u, n)
            nfor(e, g[u]) {
                to.push_back(e.to);
                w.push_back(e.w);
            }
    }
    int len() const { return n; }
    int edges() const { return int(to.size()); }
    bool empty() const { return !n; }
    template <bool C> struct cursor_t {
        using graph_type = conditional_t<C, const ngraph_csr, ngraph_csr>;
        using weight_type = conditional_t<C, const W&, W&>;
        graph_type* g;
        int u, e, r, k = 0;
        bool ok() const { return e < r; }
        nedge<weight_type> val() const { return {u, g->to[e], e, g->w[e]}; }
        int idx() const { return k; }
        void next() {
            ++e;
            ++k;
        }
    };
    template <bool C> struct view_t {
        using graph_type = conditional_t<C, const ngraph_csr, ngraph_csr>;
        graph_type* g;
        int u;
        auto enumerate() const { return cursor_t<C>{g, u, g->h[u], g->h[u + 1]}; }
        int len() const { return g->h[u + 1] - g->h[u]; }
    };
    using view = view_t<false>;
    using const_view = view_t<true>;
    view operator[](int u) & {
        nassert(0 <= u && u < n);
        return {this, u};
    }
    const_view operator[](int u) const& {
        nassert(0 <= u && u < n);
        return {this, u};
    }
    view operator[](int) && = delete;
    const_view operator[](int) const&& = delete;
    view from(int u) & { return (*this)[u]; }
    const_view from(int u) const& { return (*this)[u]; }
    view from(int) && = delete;
    const_view from(int) const&& = delete;
    int degree(int u) const { return from(u).len(); }
    int find(int u, int v, int d = npos) const {
        nassert(0 <= u && u < n && 0 <= v && v < n);
        nfor(e, (*this)[u])
            if (e.to == v)
                return e.id;
        return d;
    }
    bool has(int u, int v) const { return find(u, v) != npos; }
    W* weight(int e) { return 0 <= e && e < edges() ? &w[e] : nullptr; }
    const W* weight(int e) const { return 0 <= e && e < edges() ? &w[e] : nullptr; }
    W weight(int e, W d) const { return 0 <= e && e < edges() ? w[e] : move(d); }
    bool set(int e, W z) {
        if (e < 0 || e >= edges())
            return false;
        w[e] = move(z);
        return true;
    }
    auto vertices() const { return nvertices(*this); }
    auto arcs() & { return narcs(*this); }
    auto arcs() const& { return narcs(*this); }
    auto arcs() && = delete;
    auto arcs() const&& = delete;
    ngraph_csr reverse() const {
        ngraph_forward<W> z(n, edges());
        nrep(u, n)
            nfor(e, (*this)[u])
                z.add(e.to, u, e.w);
        return ngraph_csr(z);
    }
};
template <class G> ngraph_csr(const G&) -> ngraph_csr<ni::ngraph_weight_t<G>>;

template <class D> struct npath_result {
    nvector_stl<D> d;
    nvector_stl<int> p;
    D bad{};
    int len() const { return d.len(); }
    bool reach(int v) const { return 0 <= v && v < len() && v < p.len() && p[v] != npos; }
    D dist(int v, D x) const { return reach(v) ? d[v] : move(x); }
    const D& operator[](int v) const {
        nassert(0 <= v && v < len());
        return d[v];
    }
    nvector_stl<int> path(int v) const {
        nvector_stl<int> z;
        if (!reach(v) || p[v] == npos)
            return z;
        for (int u = v;; u = p[u]) {
            z.push(u);
            if (p[u] == u)
                break;
        }
        reverse(z.a.begin(), z.a.end());
        return z;
    }
};
template <class G> npath_result<int> nbfs(const G& g, int s) {
    nassert(0 <= s && s < g.len());
    npath_result<int> r{nvector_stl<int>(g.len(), npos), nvector_stl<int>(g.len(), npos), npos};
    queue<int> q;
    r.d[s] = 0;
    r.p[s] = s;
    q.push(s);
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        nfor(e, g[u])
            if (r.d[e.to] == npos)
                r.d[e.to] = r.d[u] + 1, r.p[e.to] = u, q.push(e.to);
    }
    return r;
}
template <class G, class F>
    requires invocable<F, ni::ngraph_edge_t<G>>
npath_result<int> n01bfs(const G& g, int s, F weight, int inf = ninf<int>) {
    nassert(0 <= s && s < g.len());
    npath_result<int> r{nvector_stl<int>(g.len(), inf), nvector_stl<int>(g.len(), npos), inf};
    deque<int> q;
    vector<char> vis(g.len());
    r.d[s] = 0;
    r.p[s] = s;
    q.push_back(s);
    while (!q.empty()) {
        int u = q.front();
        q.pop_front();
        if (vis[u])
            continue;
        vis[u] = 1;
        nfor(e, g[u]) {
            int x = int(invoke(weight, e));
            nassert(x == 0 || x == 1);
            int z = r.d[u] + x;
            if (z < r.d[e.to])
                r.d[e.to] = z, r.p[e.to] = u, x ? q.push_back(e.to) : q.push_front(e.to);
        }
    }
    return r;
}
template <class G> npath_result<int> n01bfs(const G& g, int s, int inf = ninf<int>) {
    return n01bfs(g, s, [](auto&& e) -> decltype(auto) { return e.w; }, inf);
}
template <class G, class F, class W = remove_cvref_t<invoke_result_t<F, ni::ngraph_edge_t<G>>>>
    requires invocable<F, ni::ngraph_edge_t<G>>
npath_result<W> ndijkstra(const G& g, int s, F weight, W inf = ninf<W>) {
    nassert(0 <= s && s < g.len());
    npath_result<W> r{nvector_stl<W>(g.len(), inf), nvector_stl<int>(g.len(), npos), inf};
    priority_queue<pair<W, int>, vector<pair<W, int>>, greater<pair<W, int>>> q;
    r.d[s] = W{};
    r.p[s] = s;
    q.push({W{}, s});
    while (!q.empty()) {
        auto [d, u] = q.top();
        q.pop();
        if (d != r.d[u])
            continue;
        nfor(e, g[u]) {
            W x = invoke(weight, e);
            nassert(!(x < W{}));
            W z = ncapadd(d, x, inf);
            if (z < r.d[e.to])
                r.d[e.to] = z, r.p[e.to] = u, q.push({z, e.to});
        }
    }
    return r;
}
template <class G> npath_result<ni::ngraph_weight_t<G>>
ndijkstra(const G& g, int s, ni::ngraph_weight_t<G> inf = ninf<ni::ngraph_weight_t<G>>) {
    return ndijkstra(g, s, [](auto&& e) -> decltype(auto) { return e.w; }, move(inf));
}
template <class G> nmaybe<nvector_stl<int>> ntopo(const G& g) {
    vector<int> d(g.len());
    nrep(u, g.len())
        nfor(e, g[u])
            ++d[e.to];
    queue<int> q;
    nrep(i, g.len())
        if (!d[i])
            q.push(i);
    nvector_stl<int> z;
    z.reserve(g.len());
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        z.push(u);
        nfor(e, g[u])
            if (!--d[e.to])
                q.push(e.to);
    }
    return z.len() == g.len() ? nmaybe<nvector_stl<int>>(move(z)) : nmaybe<nvector_stl<int>>{};
}
template <class G> nvector_stl<int> ntopo(const G& g, nvector_stl<int> d) {
    auto z = ntopo(g);
    return z ? move(z.val()) : move(d);
}
template <class G> npart_dense nscc_tarjan(const G& g) {
    int n = g.len(), tm = 0, cc = 0;
    vector<int> df(n), lo(n), st;
    vector<char> on(n);
    nvector_stl<int> c(n, npos);
    auto dfs = [&](auto&& dfs, int u) -> void {
        df[u] = lo[u] = ++tm;
        st.push_back(u);
        on[u] = 1;
        nfor(e, g[u])
            if (!df[e.to])
                dfs(dfs, e.to), nchmin(lo[u], lo[e.to]);
            else if (on[e.to])
                nchmin(lo[u], df[e.to]);
        if (lo[u] == df[u]) {
            for (;;) {
                int v = st.back();
                st.pop_back();
                on[v] = 0;
                c[v] = cc;
                if (v == u)
                    break;
            }
            ++cc;
        }
    };
    nrep(i, n)
        if (!df[i])
            dfs(dfs, i);
    return npart_dense(move(c));
}
template <class G> npart_dense nscc_kosaraju(const G& g) {
    int n = g.len(), cc = 0;
    vector<char> vis(n);
    vector<int> ord;
    ord.reserve(n);
    using cursor = decltype(nenumerate(g[0]));
    nrep(s, n)
        if (!vis[s]) {
            vector<pair<int, cursor>> st;
            st.emplace_back(s, nenumerate(g[s]));
            vis[s] = 1;
            while (!st.empty()) {
                int u = st.back().first;
                auto& e = st.back().second;
                while (e.ok() && vis[e.val().to])
                    e.next();
                if (!e.ok())
                    ord.push_back(u), st.pop_back();
                else {
                    int v = e.val().to;
                    e.next();
                    vis[v] = 1;
                    st.emplace_back(v, nenumerate(g[v]));
                }
            }
        }
    ngraph_forward<> r(n);
    if constexpr (requires { g.edges(); })
        r.reserve(g.edges());
    nrep(u, n)
        nfor(e, g[u])
            r.add(e.to, u);
    nvector_stl<int> c(n, npos);
    for (int i = n; i--;) {
        int s = ord[i];
        if (c[s] != npos)
            continue;
        vector<int> st{s};
        c[s] = cc;
        while (!st.empty()) {
            int u = st.back();
            st.pop_back();
            nfor(e, r[u])
                if (c[e.to] == npos)
                    c[e.to] = cc, st.push_back(e.to);
        }
        ++cc;
    }
    return npart_dense(move(c));
}

template <class W> struct nmst_result {
    W cost{};
    nvector_stl<int> edge;
    int components = 0;
    bool connected() const { return components <= 1; }
};
template <class G, class F, class W = remove_cvref_t<invoke_result_t<F, ni::ngraph_edge_t<G>>>>
    requires invocable<F, ni::ngraph_edge_t<G>>
nmst_result<W> nkruskal(const G& g, F weight) {
    struct E {
        int u, v, id;
        W w;
    };
    vector<E> a;
    if constexpr (requires { g.edges(); })
        a.reserve(g.edges());
    nrep(u, g.len())
        nfor(e, g[u])
            a.push_back({u, e.to, e.id, invoke(weight, e)});
    sort(a.begin(), a.end(), [](const E& x, const E& y) { return x.w < y.w; });
    ndsu d(g.len());
    nmst_result<W> z;
    z.components = g.len();
    for (auto& e : a)
        if (d.unite(e.u, e.v))
            z.cost += e.w, z.edge.push(e.id), --z.components;
    return z;
}
template <class G> nmst_result<ni::ngraph_weight_t<G>> nkruskal(const G& g) {
    return nkruskal(g, [](auto&& e) -> decltype(auto) { return e.w; });
}

template <class W = int> struct nlca_binary {
    int n = 0, L = 0;
    vector<vector<int>> up;
    vector<int> dep, comp;
    vector<W> dis;
    nlca_binary() = default;
    template <class G> explicit nlca_binary(const G& g, int root = 0) {
        build(g, root, [](auto&& e) -> decltype(auto) { return e.w; });
    }
    template <class G, class F>
        requires invocable<F, ni::ngraph_edge_t<G>>
    nlca_binary(const G& g, int root, F weight) {
        build(g, root, move(weight));
    }
    template <class G, class F> void build(const G& g, int root, F weight) {
        n = g.len();
        L = bit_width(unsigned(max(1, n)));
        up.assign(L, vector<int>(n));
        dep.assign(n, npos);
        comp.assign(n, npos);
        dis.assign(n, W{});
        if (n)
            nassert(0 <= root && root < n);
        vector<int> st;
        if (n)
            st.push_back(root);
        nrep(i, n)
            if (i != root)
                st.push_back(i);
        for (int s : st)
            if (dep[s] == npos) {
                queue<int> q;
                dep[s] = 0;
                comp[s] = s;
                up[0][s] = s;
                q.push(s);
                while (!q.empty()) {
                    int u = q.front();
                    q.pop();
                    nfor(e, g[u])
                        if (dep[e.to] == npos)
                            dep[e.to] = dep[u] + 1, comp[e.to] = s, up[0][e.to] = u,
                            dis[e.to] = dis[u] + W(invoke(weight, e)), q.push(e.to);
                }
            }
        for (int k = 1; k < L; ++k)
            nrep(i, n)
                up[k][i] = up[k - 1][up[k - 1][i]];
    }
    int len() const { return n; }
    bool same(int a, int b) const { return 0 <= a && a < n && 0 <= b && b < n && comp[a] == comp[b]; }
    int jump(int v, int k, int d = npos) const {
        if (v < 0 || v >= n || k < 0 || k > dep[v])
            return d;
        for (int b = 0; k; k >>= 1, ++b)
            if (k & 1)
                v = up[b][v];
        return v;
    }
    int lca(int a, int b, int d = npos) const {
        if (!same(a, b))
            return d;
        if (dep[a] < dep[b])
            swap(a, b);
        a = jump(a, dep[a] - dep[b]);
        if (a == b)
            return a;
        for (int k = L; k--;)
            if (up[k][a] != up[k][b])
                a = up[k][a], b = up[k][b];
        return up[0][a];
    }
    int kth(int a, int b, int k, int d = npos) const {
        int c = lca(a, b);
        if (c == npos)
            return d;
        int x = dep[a] - dep[c], y = dep[b] - dep[c];
        return k < 0 || k > x + y ? d : k <= x ? jump(a, k) : jump(b, x + y - k);
    }
    nmaybe<W> dist(int a, int b) const {
        int c = lca(a, b);
        return c == npos ? nmaybe<W>{} : nmaybe<W>(dis[a] + dis[b] - dis[c] - dis[c]);
    }
    W dist(int a, int b, W d) const {
        auto z = dist(a, b);
        return z ? z.val() : move(d);
    }
};
template <class G> nlca_binary(const G&, int = 0) -> nlca_binary<ni::ngraph_weight_t<G>>;
template <class G, class F>
nlca_binary(const G&, int, F) -> nlca_binary<remove_cvref_t<invoke_result_t<F, ni::ngraph_edge_t<G>>>>;

struct nhld_segment {
    int l, r;
    bool rev;
    friend bool operator==(const nhld_segment&, const nhld_segment&) = default;
};
template <class W = int> struct nhld {
    int n = 0;
    vector<int> par, dep, sz, heavy, head, pos, inv, comp, roots;
    nhld() = default;
    template <class G> explicit nhld(const G& g, int root = 0) {
        n = g.len();
        par.assign(n, npos);
        dep.assign(n, 0);
        sz.assign(n, 1);
        heavy.assign(n, npos);
        head.resize(n);
        pos.resize(n);
        inv.resize(n);
        comp.assign(n, npos);
        if (n)
            nassert(0 <= root && root < n);
        vector<int> start, ord;
        if (n)
            start.push_back(root);
        nrep(i, n)
            if (i != root)
                start.push_back(i);
        for (int s : start)
            if (par[s] == npos) {
                roots.push_back(s);
                par[s] = s;
                comp[s] = s;
                vector<int> st{s};
                while (!st.empty()) {
                    int u = st.back();
                    st.pop_back();
                    ord.push_back(u);
                    nfor(e, g[u])
                        if (par[e.to] == npos)
                            par[e.to] = u, dep[e.to] = dep[u] + 1, comp[e.to] = s, st.push_back(e.to);
                }
            }
        for (int i = int(ord.size()); i--;) {
            int u = ord[i];
            if (par[u] != u) {
                int p = par[u];
                sz[p] += sz[u];
                if (heavy[p] == npos || sz[u] > sz[heavy[p]])
                    heavy[p] = u;
            }
        }
        int tm = 0;
        vector<pair<int, int>> st;
        for (int r : roots)
            st.push_back({r, r});
        while (!st.empty()) {
            auto [s, h] = st.back();
            st.pop_back();
            for (int u = s; u != npos; u = heavy[u]) {
                head[u] = h;
                pos[u] = tm;
                inv[tm++] = u;
                nfor(e, g[u])
                    if (par[e.to] == u && e.to != heavy[u])
                        st.push_back({e.to, e.to});
            }
        }
    }
    int len() const { return n; }
    bool same(int a, int b) const { return 0 <= a && a < n && 0 <= b && b < n && comp[a] == comp[b]; }
    int operator()(int v) const {
        nassert(0 <= v && v < n);
        return pos[v];
    }
    int vertex(int i, int d = npos) const { return 0 <= i && i < n ? inv[i] : d; }
    pair<int, int> subtree(int u) const {
        nassert(0 <= u && u < n);
        return {pos[u], pos[u] + sz[u]};
    }
    int lca(int a, int b, int d = npos) const {
        if (!same(a, b))
            return d;
        while (head[a] != head[b])
            dep[head[a]] > dep[head[b]] ? a = par[head[a]] : b = par[head[b]];
        return dep[a] < dep[b] ? a : b;
    }
    nvector_stl<nhld_segment> path(int a, int b, bool edge = false) const {
        nvector_stl<nhld_segment> l, r;
        if (!same(a, b))
            return l;
        while (head[a] != head[b])
            if (dep[head[a]] >= dep[head[b]])
                l.push(nhld_segment{pos[head[a]], pos[a] + 1, true}), a = par[head[a]];
            else
                r.push(nhld_segment{pos[head[b]], pos[b] + 1, false}), b = par[head[b]];
        if (dep[a] >= dep[b]) {
            int x = pos[b] + edge;
            if (x <= pos[a])
                l.push(nhld_segment{x, pos[a] + 1, true});
        } else {
            int x = pos[a] + edge;
            if (x <= pos[b])
                r.push(nhld_segment{x, pos[b] + 1, false});
        }
        for (int i = r.len(); i--;)
            l.push(r[i]);
        return l;
    }
    template <class F> bool each(int a, int b, F f, bool edge = false) const {
        auto z = path(a, b, edge);
        if (!same(a, b))
            return false;
        nfor(s, z)
            f(s.l, s.r, s.rev);
        return true;
    }
};
template <class G> nhld(const G&, int = 0) -> nhld<ni::ngraph_weight_t<G>>;

template <class T> struct nflow_dinic {
    int n = 0;
    vector<int> h, to, nx, lv, it;
    vector<T> cap, base;
    nflow_dinic() = default;
    explicit nflow_dinic(int n, int m = 0)
        : n(max(0, n)), h(size_t(max(0, n)), -1), lv(size_t(max(0, n))), it(size_t(max(0, n))) {
        nassert(n >= 0);
        reserve(m);
    }
    int len() const { return n; }
    int edges() const { return int(to.size() / 2); }
    void reserve(int m) {
        nassert(m >= 0);
        if (m < 0)
            return;
        to.reserve(size_t(2) * size_t(m));
        nx.reserve(size_t(2) * size_t(m));
        cap.reserve(size_t(2) * size_t(m));
        base.reserve(size_t(2) * size_t(m));
    }
    int add(int u, int v, T c, T r = T{}) {
        nassert(0 <= u && u < n && 0 <= v && v < n && !(c < T{}) && !(r < T{}));
        int id = edges();
        to.push_back(v);
        cap.push_back(c);
        base.push_back(c);
        nx.push_back(h[u]);
        h[u] = int(to.size()) - 1;
        to.push_back(u);
        cap.push_back(r);
        base.push_back(r);
        nx.push_back(h[v]);
        h[v] = int(to.size()) - 1;
        return id;
    }
    bool bfs(int s, int t) {
        fill(lv.begin(), lv.end(), -1);
        queue<int> q;
        lv[s] = 0;
        q.push(s);
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            for (int e = h[u]; e != npos; e = nx[e])
                if (cap[e] > T{} && lv[to[e]] < 0)
                    lv[to[e]] = lv[u] + 1, q.push(to[e]);
        }
        return lv[t] >= 0;
    }
    T dfs(int u, int t, T f) {
        if (u == t)
            return f;
        for (int& e = it[u]; e != npos; e = nx[e])
            if (cap[e] > T{} && lv[to[e]] == lv[u] + 1) {
                T z = dfs(to[e], t, min(f, cap[e]));
                if (z > T{})
                    return cap[e] -= z, cap[e ^ 1] += z, z;
            }
        return T{};
    }
    T flow(int s, int t, T lim = numeric_limits<T>::max()) {
        nassert(0 <= s && s < n && 0 <= t && t < n && s != t);
        T z{};
        while (z < lim && bfs(s, t)) {
            it = h;
            while (z < lim) {
                T f = dfs(s, t, lim - z);
                if (!(f > T{}))
                    break;
                z += f;
            }
        }
        return z;
    }
    T operator()(int s, int t, T lim = numeric_limits<T>::max()) { return flow(s, t, lim); }
    T used(int id) const {
        nassert(0 <= id && id < edges());
        return base[id * 2] - cap[id * 2];
    }
    vector<char> cut(int s) {
        vector<char> v(n);
        queue<int> q;
        v[s] = 1;
        q.push(s);
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            for (int e = h[u]; e != npos; e = nx[e])
                if (cap[e] > T{} && !v[to[e]])
                    v[to[e]] = 1, q.push(to[e]);
        }
        return v;
    }
    void reset() { cap = base; }
};

struct nbicover {
    nvector_stl<int> l, r;
};
struct nbimatch_hopcroft {
    int L = 0, R = 0;
    vector<vector<int>> g;
    vector<int> ml, mr, d;
    bool solved = false;
    nbimatch_hopcroft() = default;
    nbimatch_hopcroft(int l, int r)
        : L(max(0, l)), R(max(0, r)), g(size_t(max(0, l))), ml(size_t(max(0, l)), npos), mr(size_t(max(0, r)), npos),
          d(size_t(max(0, l))) {
        nassert(l >= 0 && r >= 0);
    }
    int add(int l, int r) {
        nassert(0 <= l && l < L && 0 <= r && r < R);
        solved = false;
        g[l].push_back(r);
        return int(g[l].size()) - 1;
    }
    bool bfs() {
        queue<int> q;
        bool ok = false;
        nrep(u, L)
            if (ml[u] == npos)
                d[u] = 0, q.push(u);
            else
                d[u] = npos;
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            for (int v : g[u])
                if (mr[v] == npos)
                    ok = true;
                else if (d[mr[v]] == npos)
                    d[mr[v]] = d[u] + 1, q.push(mr[v]);
        }
        return ok;
    }
    bool dfs(int u) {
        for (int v : g[u])
            if (mr[v] == npos || (d[mr[v]] == d[u] + 1 && dfs(mr[v])))
                return ml[u] = v, mr[v] = u, true;
        d[u] = npos;
        return false;
    }
    int solve() {
        fill(ml.begin(), ml.end(), npos);
        fill(mr.begin(), mr.end(), npos);
        int z = 0;
        while (bfs())
            nrep(u, L)
                if (ml[u] == npos)
                    z += dfs(u);
        solved = true;
        return z;
    }
    int left(int u, int x = npos) const {
        nassert(0 <= u && u < L);
        return ml[u] == npos ? x : ml[u];
    }
    int right(int v, int x = npos) const {
        nassert(0 <= v && v < R);
        return mr[v] == npos ? x : mr[v];
    }
    nvector_stl<pair<int, int>> pairs() const {
        nassert(solved);
        nvector_stl<pair<int, int>> z;
        nrep(u, L)
            if (ml[u] != npos)
                z.push(pair<int, int>{u, ml[u]});
        return z;
    }
    nbicover mincover() const {
        nassert(solved);
        vector<char> a(L), b(R);
        queue<int> q;
        nrep(u, L)
            if (ml[u] == npos)
                a[u] = 1, q.push(u);
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            for (int v : g[u])
                if (ml[u] != v && !b[v]) {
                    b[v] = 1;
                    if (mr[v] != npos && !a[mr[v]])
                        a[mr[v]] = 1, q.push(mr[v]);
                }
        }
        nbicover z;
        nrep(u, L)
            if (!a[u])
                z.l.push(u);
        nrep(v, R)
            if (b[v])
                z.r.push(v);
        return z;
    }
};

// 41 nstring
template <class A> nvector_stl<int> nzfunc(const A& s) {
    int n = nlen(s);
    nvector_stl<int> z(n);
    if (!n)
        return z;
    z[0] = n;
    for (int i = 1, l = 0, r = 0; i < n; ++i) {
        if (i < r)
            z[i] = min(r - i, z[i - l]);
        while (i + z[i] < n && s[z[i]] == s[i + z[i]])
            ++z[i];
        if (i + z[i] > r)
            l = i, r = i + z[i];
    }
    return z;
}
template <class A> nvector_stl<int> nprefix(const A& s) {
    int n = nlen(s);
    nvector_stl<int> p(n);
    for (int i = 1; i < n; ++i) {
        int j = p[i - 1];
        while (j && !(s[i] == s[j]))
            j = p[j - 1];
        if (s[i] == s[j])
            ++j;
        p[i] = j;
    }
    return p;
}
template <class A, class B> nvector_stl<int> nkmp(const A& s, const B& p) {
    int n = nlen(s), m = nlen(p);
    nvector_stl<int> z;
    if (!m) {
        z.resize(n + 1);
        nrep(i, n + 1)
            z[i] = i;
        return z;
    }
    auto f = nprefix(p);
    for (int i = 0, j = 0; i < n; ++i) {
        while (j && !(s[i] == p[j]))
            j = f[j - 1];
        if (s[i] == p[j])
            ++j;
        if (j == m)
            z.push(i - m + 1), j = f[j - 1];
    }
    return z;
}
struct nmanacher_result {
    nvector_stl<int> odd, even;
    int len() const { return odd.len(); }
    bool pal(int l, int r) const {
        nassert(0 <= l && l <= r && r <= len());
        int n = r - l;
        if (n & 1)
            return odd[(l + r) / 2] >= n / 2 + 1;
        return !n || even[(l + r) / 2] >= n / 2;
    }
};
template <class A> nmanacher_result nmanacher(const A& s) {
    int n = nlen(s);
    nmanacher_result z{nvector_stl<int>(n), nvector_stl<int>(n)};
    for (int i = 0, l = 0, r = -1; i < n; ++i) {
        int k = i > r ? 1 : min(z.odd[l + r - i], r - i + 1);
        while (i - k >= 0 && i + k < n && s[i - k] == s[i + k])
            ++k;
        z.odd[i] = k--;
        if (i + k > r)
            l = i - k, r = i + k;
    }
    for (int i = 0, l = 0, r = -1; i < n; ++i) {
        int k = i > r ? 0 : min(z.even[l + r - i + 1], r - i + 1);
        while (i - k - 1 >= 0 && i + k < n && s[i - k - 1] == s[i + k])
            ++k;
        z.even[i] = k--;
        if (i + k > r)
            l = i - k - 1, r = i + k;
    }
    return z;
}

template <class A, class C = nless<>> nvector_stl<int> nsuffix_array(const A& s, C cmp = {}) {
    int n = nlen(s);
    nvector_stl<int> z(n);
    auto& sa = z.a;
    if (!n)
        return z;
    iota(sa.begin(), sa.end(), 0);
    sort(sa.begin(), sa.end(), [&](int i, int j) { return cmp(s[i], s[j]); });
    vector<int> r(n), nr(n), y(n), cnt;
    int k = 1;
    r[sa[0]] = 0;
    for (int i = 1; i < n; ++i)
        r[sa[i]] = r[sa[i - 1]] + (cmp(s[sa[i - 1]], s[sa[i]]) || cmp(s[sa[i]], s[sa[i - 1]]));
    k = r[sa[n - 1]] + 1;
    for (int d = 1; k<n; d = d> n / 2 ? n : d * 2) {
        int q = 0;
        for (int i = n - d; i < n; ++i)
            y[q++] = i;
        for (int x : sa)
            if (x >= d)
                y[q++] = x - d;
        cnt.assign(k, 0);
        for (int x : y)
            ++cnt[r[x]];
        partial_sum(cnt.begin(), cnt.end(), cnt.begin());
        for (int i = n; i--;)
            sa[--cnt[r[y[i]]]] = y[i];
        nr[sa[0]] = 0;
        for (int i = 1; i < n; ++i) {
            int a = sa[i - 1], b = sa[i];
            nr[b] = nr[a] + (r[a] != r[b] || (a + d < n ? r[a + d] : -1) != (b + d < n ? r[b + d] : -1));
        }
        r.swap(nr);
        k = r[sa[n - 1]] + 1;
    }
    return z;
}
template <class A> nvector_stl<int> nlcp_array(const A& s, const nvector_stl<int>& sa) {
    int n = nlen(s);
    nassert(sa.len() == n);
    nvector_stl<int> z(n), r(n);
    nrep(i, n)
        r[sa[i]] = i;
    for (int i = 0, h = 0; i < n; ++i) {
        int k = r[i];
        if (k) {
            int j = sa[k - 1];
            while (i + h < n && j + h < n && s[i + h] == s[j + h])
                ++h;
            z[k] = h;
            if (h)
                --h;
        }
    }
    return z;
}

struct nmatch {
    int l, r, id;
    friend bool operator==(const nmatch&, const nmatch&) = default;
};
template <int K = 26, int Base = 'a'> struct nac {
    static_assert(K > 0);
    struct node {
        array<int, K> go{};
        int fail = 0, out = npos, head = npos;
    };
    vector<node> t{1};
    vector<int> oid, onx, plen, ord;
    bool built = false;
    int code(auto c) const {
        long long x = static_cast<long long>(c) - Base;
        return 0 <= x && x < K ? int(x) : npos;
    }
    int nodes() const { return int(t.size()); }
    int patterns() const { return int(plen.size()); }
    template <class A> int add(const A& s, int id = npos) {
        nassert(!built && nlen(s));
        if (id == npos)
            id = patterns();
        nassert(id >= 0);
        if (id >= patterns())
            plen.resize(id + 1, npos);
        nassert(plen[id] == npos);
        int u = 0;
        nrep(i, nlen(s)) {
            int c = code(s[i]);
            nassert(c != npos);
            if (!t[u].go[c])
                t[u].go[c] = nodes(), t.push_back({});
            u = t[u].go[c];
        }
        plen[id] = nlen(s);
        oid.push_back(id);
        onx.push_back(t[u].head);
        t[u].head = int(oid.size()) - 1;
        return id;
    }
    void build() {
        if (built)
            return;
        built = true;
        queue<int> q;
        ord = {0};
        nrep(c, K)
            if (int v = t[0].go[c])
                q.push(v), ord.push_back(v);
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            nrep(c, K)
                if (int v = t[u].go[c]) {
                    int f = t[t[u].fail].go[c];
                    t[v].fail = f;
                    t[v].out = t[f].head != npos ? f : t[f].out;
                    q.push(v);
                    ord.push_back(v);
                } else
                    t[u].go[c] = t[t[u].fail].go[c];
        }
    }
    int step(int u, auto c, int d = 0) const {
        nassert(built && 0 <= u && u < nodes());
        int x = code(c);
        return x == npos ? d : t[u].go[x];
    }
    template <class A, class F> void each(const A& s, F f) const {
        nassert(built);
        for (int i = 0, u = 0; i < nlen(s); ++i) {
            u = step(u, s[i]);
            for (int v = u; v != npos; v = t[v].out)
                for (int e = t[v].head; e != npos; e = onx[e]) {
                    int id = oid[e];
                    f(i + 1 - plen[id], i + 1, id);
                }
        }
    }
    template <class A> nvector_stl<nmatch> matches(const A& s) const {
        nvector_stl<nmatch> z;
        each(s, [&](int l, int r, int id) { z.push(nmatch{l, r, id}); });
        return z;
    }
    template <class A> nvector_stl<long long> count(const A& s) const {
        nassert(built);
        vector<long long> c(nodes());
        for (int u = 0, i = 0; i < nlen(s); ++i)
            u = step(u, s[i]), ++c[u];
        for (int i = int(ord.size()); --i > 0;)
            c[t[ord[i]].fail] += c[ord[i]];
        nvector_stl<long long> z(patterns());
        nrep(u, nodes())
            for (int e = t[u].head; e != npos; e = onx[e])
                z[oid[e]] = c[u];
        return z;
    }
};

// 42 ngeom
template <class T> struct npoint {
    T x{}, y{};
    constexpr npoint() = default;
    constexpr npoint(T x, T y) : x(x), y(y) {}
    template <class U> explicit constexpr operator npoint<U>() const { return {U(x), U(y)}; }
    constexpr npoint& operator+=(const npoint& p) {
        x += p.x;
        y += p.y;
        return *this;
    }
    constexpr npoint& operator-=(const npoint& p) {
        x -= p.x;
        y -= p.y;
        return *this;
    }
    template <class U> constexpr npoint& operator*=(U k) {
        x *= k;
        y *= k;
        return *this;
    }
    template <class U> constexpr npoint& operator/=(U k) {
        x /= k;
        y /= k;
        return *this;
    }
    constexpr npoint operator+() const { return *this; }
    constexpr npoint operator-() const { return {-x, -y}; }
    friend constexpr npoint operator+(npoint a, const npoint& b) { return a += b; }
    friend constexpr npoint operator-(npoint a, const npoint& b) { return a -= b; }
    template <class U> friend constexpr npoint operator*(npoint a, U k) { return a *= k; }
    template <class U> friend constexpr npoint operator*(U k, npoint a) { return a *= k; }
    template <class U> friend constexpr npoint operator/(npoint a, U k) { return a /= k; }
    friend constexpr auto operator<=>(const npoint&, const npoint&) = default;
};
template <class T> constexpr nwide_t<T> ndot(const npoint<T>& a, const npoint<T>& b) {
    using W = nwide_t<T>;
    return W(a.x) * b.x + W(a.y) * b.y;
}
template <class T> constexpr nwide_t<T> ncross(const npoint<T>& a, const npoint<T>& b) {
    using W = nwide_t<T>;
    return W(a.x) * b.y - W(a.y) * b.x;
}
template <class T> constexpr nwide_t<T> ncross(const npoint<T>& a, const npoint<T>& b, const npoint<T>& c) {
    using W = nwide_t<T>;
    return (W(b.x) - a.x) * (W(c.y) - a.y) - (W(b.y) - a.y) * (W(c.x) - a.x);
}
template <class T> constexpr nwide_t<T> ndist2(const npoint<T>& a, const npoint<T>& b) {
    using W = nwide_t<T>;
    W x = W(a.x) - b.x, y = W(a.y) - b.y;
    return x * x + y * y;
}
template <class X> constexpr int nsgn_eps(X x, long double e = 0) {
    if constexpr (is_integral_v<X> || is_same_v<X, __int128_t> || is_same_v<X, __uint128_t>)
        return x > 0 ? 1 : x < 0 ? -1 : 0;
    else
        return x > e ? 1 : x < -e ? -1 : 0;
}
template <class T>
constexpr int norient(const npoint<T>& a, const npoint<T>& b, const npoint<T>& c, long double e = 0) {
    return nsgn_eps(ncross(a, b, c), e);
}
template <class T>
constexpr bool nonseg(const npoint<T>& a, const npoint<T>& b, const npoint<T>& p, long double e = 0) {
    if (norient(a, b, p, e))
        return false;
    if constexpr (integral<T>)
        return min(a.x, b.x) <= p.x && p.x <= max(a.x, b.x) && min(a.y, b.y) <= p.y && p.y <= max(a.y, b.y);
    else
        return min(a.x, b.x) - e <= p.x && p.x <= max(a.x, b.x) + e && min(a.y, b.y) - e <= p.y &&
               p.y <= max(a.y, b.y) + e;
}
template <class T>
constexpr bool nsegment_intersect(const npoint<T>& a, const npoint<T>& b, const npoint<T>& c, const npoint<T>& d,
                                  long double e = 0) {
    int x = norient(a, b, c, e), y = norient(a, b, d, e), z = norient(c, d, a, e), w = norient(c, d, b, e);
    return (x && y && z && w) ? x != y && z != w
                              : (!x && nonseg(a, b, c, e)) || (!y && nonseg(a, b, d, e)) ||
                                    (!z && nonseg(c, d, a, e)) || (!w && nonseg(c, d, b, e));
}
template <class T> struct nline2 {
    npoint<T> p, v;
    npoint<T> operator()(T k) const { return p + v * k; }
};
template <class T>
nmaybe<npoint<long double>> nline_intersect(const nline2<T>& a, const nline2<T>& b, long double e = 0) {
    long double d = static_cast<long double>(ncross(a.v, b.v));
    if (abs(d) <= e)
        return {};
    npoint<long double> x = static_cast<npoint<long double>>(b.p - a.p), u = static_cast<npoint<long double>>(a.v),
                        v = static_cast<npoint<long double>>(b.v), p = static_cast<npoint<long double>>(a.p);
    return p + u * (ncross(x, v) / ncross(u, v));
}
template <class A> auto npolygon_area2(const A& p) {
    using T = remove_cvref_t<decltype(p[0].x)>;
    using W = nwide_t<T>;
    W z{};
    int n = nlen(p);
    nrep(i, n)
        z += ncross(p[i], p[(i + 1) % n]);
    return z;
}
template <class A> auto nconvex_hull(const A& p, bool keep = false) {
    using P = remove_cvref_t<decltype(p[0])>;
    nvector_stl<P> a;
    nfor(x, p)
        a.push(x);
    nsort(a);
    nunique(a);
    int n = a.len();
    if (n < 3)
        return a;
    if (keep) {
        bool col = true;
        for (int i = 2; i < n; ++i)
            col &= !norient(a[0], a[1], a[i]);
        if (col)
            return a;
    }
    nvector_stl<P> h;
    h.reserve(n * 2);
    nrep(i, n) {
        while (h.len() > 1 &&
               (keep ? norient(h[h.len() - 2], h.back(), a[i]) < 0 : norient(h[h.len() - 2], h.back(), a[i]) <= 0))
            h.pop();
        h.push(a[i]);
    }
    int k = h.len();
    for (int i = n - 2; i >= 0; --i) {
        while (h.len() > k &&
               (keep ? norient(h[h.len() - 2], h.back(), a[i]) < 0 : norient(h[h.len() - 2], h.back(), a[i]) <= 0))
            h.pop();
        h.push(a[i]);
    }
    h.pop();
    return h;
}
template <class A> int npoint_in_poly(const A& p, const remove_cvref_t<decltype(p[0])>& x, long double e = 0) {
    bool in = false;
    int n = nlen(p);
    nrep(i, n) {
        auto& a = p[i];
        auto& b = p[(i + 1) % n];
        if (nonseg(a, b, x, e))
            return 0;
        int o = norient(a, b, x, e);
        if ((a.y <= x.y && x.y < b.y && o > 0) || (b.y <= x.y && x.y < a.y && o < 0))
            in = !in;
    }
    return in ? 1 : -1;
}
template <class A> auto nconvex_diameter2(const A& p) {
    using T = remove_cvref_t<decltype(p[0].x)>;
    using W = nwide_t<T>;
    int n = nlen(p);
    if (n < 2)
        return W{};
    if (n == 2)
        return ndist2(p[0], p[1]);
    W z{};
    for (int i = 0, j = 1; i < n; ++i) {
        int k = (i + 1) % n;
        while (ncross(p[k] - p[i], p[(j + 1) % n] - p[i]) > ncross(p[k] - p[i], p[j] - p[i]))
            j = (j + 1) % n;
        nchmax(z, ndist2(p[i], p[j]));
        nchmax(z, ndist2(p[k], p[j]));
    }
    return z;
}

// 43 nprob
template <class P = long double> struct nprob {
    nvector_stl<P> p;
    nprob() = default;
    explicit nprob(int n, P x = P{}) : p(n, x) {}
    nprob(initializer_list<P> x) : p(x) {}
    int len() const { return p.len(); }
    bool empty() const { return p.empty(); }
    P& operator[](int i) { return p[i]; }
    const P& operator[](int i) const { return p[i]; }
    P get(int i, P d = P{}) const { return 0 <= i && i < len() ? p[i] : move(d); }
    P sum() const {
        P z{};
        nfor(x, p)
            z += x;
        return z;
    }
    nprob& operator*=(P x) {
        nfor(y, p)
            y *= x;
        return *this;
    }
    friend nprob operator*(nprob a, P x) { return a *= x; }
    friend nprob operator*(P x, nprob a) { return a *= x; }
    bool nonnegative() const {
        nfor(x, p)
            if (x < P{})
                return false;
        return true;
    }
    template <class F> auto expect(F f) const {
        using R = remove_cvref_t<invoke_result_t<F, int>>;
        R z{};
        nrep(i, len())
            z += p[i] * f(i);
        return z;
    }
    nmaybe<nprob> normalized(P total = P{1}) const {
        P s = sum();
        if (!nonnegative() || !(s > P{}))
            return {};
        nprob z = *this;
        nfor(x, z.p)
            x = x * total / s;
        return z;
    }
    nprob& normalize(P total = P{1}) {
        auto z = normalized(total);
        nassert(z);
        if (z)
            *this = move(z.val());
        return *this;
    }
    int draw(nrng& g = nrng_global, int d = npos) const
        requires floating_point<P>
    {
        long double s = static_cast<long double>(sum());
        if (!nonnegative() || !(s > 0))
            return d;
        long double x = static_cast<long double>(g()) / static_cast<long double>(numeric_limits<uint64_t>::max()) * s;
        for (int i = 0; i < len(); ++i)
            if ((x -= static_cast<long double>(p[i])) < 0)
                return i;
        return len() - 1;
    }
    friend bool operator==(const nprob&, const nprob&) = default;
};
template <class P, class F> auto nexpect(const nprob<P>& p, F f) {
    return p.expect(move(f));
}

// 44 ngame
template <unsigned_integral T = uint64_t> struct nnim {
    nvector_stl<T> h;
    T x{};
    nnim() = default;
    template <class A> explicit nnim(const A& a) {
        nfor(v, a)
            push(v);
    }
    int len() const { return h.len(); }
    void push(T v) {
        h.push(v);
        x ^= v;
    }
    bool win() const { return x != T{}; }
    nmaybe<pair<int, T>> winning() const {
        if (!win())
            return {};
        nrep(i, len()) {
            T y = h[i] ^ x;
            if (y < h[i])
                return pair<int, T>{i, y};
        }
        return {};
    }
    pair<int, T> winning(pair<int, T> d) const {
        auto z = winning();
        return z ? z.val() : move(d);
    }
};
template <class G> nmaybe<nvector_stl<int>> nsg_dag(const G& g) {
    auto o = ntopo(g);
    if (!o)
        return {};
    int n = g.len(), stamp = 0;
    vector<int> mark(n + 1, -1);
    nvector_stl<int> z(n);
    for (int ii = o->len(); ii--;) {
        int u = (*o)[ii];
        ++stamp;
        nfor(e, g[u])
            if (z[e.to] <= n)
                mark[z[e.to]] = stamp;
        int v = 0;
        while (v <= n && mark[v] == stamp)
            ++v;
        z[u] = v;
    }
    return z;
}
template <class G> nvector_stl<int> nsg_dag(const G& g, nvector_stl<int> d) {
    auto z = nsg_dag(g);
    return z ? move(z.val()) : move(d);
}

// 45 nopt
template <integral T, class F> constexpr T nfirst_true(T l, T r, F f) {
    nassert(l <= r);
    while (l < r) {
        T m = midpoint(l, r);
        f(m) ? r = m : l = m + 1;
    }
    return l;
}
template <integral T, class F> constexpr T nfirst_true(T l, T r, F f, T d) {
    T z = nfirst_true(l, r, f);
    return z == r && !f(r) ? move(d) : z;
}
template <integral T, class F> constexpr nmaybe<T> nlast_true(T l, T r, F f) {
    nassert(l <= r);
    T z = nfirst_true(l, r, [&](T x) { return !f(x); });
    if (z == l)
        return f(l) ? nmaybe<T>(l) : nmaybe<T>{};
    return z == r && f(r) ? nmaybe<T>(r) : nmaybe<T>(z - 1);
}
template <integral T, class F> constexpr T nlast_true(T l, T r, F f, T d) {
    auto z = nlast_true(l, r, f);
    return z ? z.val() : move(d);
}
template <floating_point T, class F> T nternary_min(T l, T r, F f, int it = 100) {
    nrep(_, it) {
        T d = (r - l) / 3, a = l + d, b = r - d;
        f(a) < f(b) ? r = b : l = a;
    }
    return midpoint(l, r);
}
template <class T> struct nline {
    T m{}, b{};
    constexpr nwide_t<T> operator()(T x) const {
        using W = nwide_t<T>;
        return W(m) * x + b;
    }
    friend bool operator==(const nline&, const nline&) = default;
};
template <class T, class C = nless<nwide_t<T>>> struct nlichao_static {
    using V = nwide_t<T>;
    struct node {
        nline<T> f{};
        bool has = false;
    };
    nvector_stl<T> x;
    vector<node> t;
    C cmp{};
    nlichao_static() = default;
    template <class A> explicit nlichao_static(const A& a, C c = {}) : cmp(move(c)) {
        nrep(i, nlen(a))
            x.push(a[i]);
        nsort_unique(x);
        t.resize(max(1, 4 * x.len()));
    }
    int len() const { return x.len(); }
    bool empty() const { return x.empty(); }
    bool hasx(const T& v) const {
        int i = nlower(x, v);
        return i < len() && !(v < x[i]) && !(x[i] < v);
    }
    void put(int u, int l, int r, nline<T> f) {
        if (!t[u].has) {
            t[u] = {f, true};
            return;
        }
        int m = (l + r) / 2;
        bool a = cmp(f(x[l]), t[u].f(x[l])), b = cmp(f(x[m]), t[u].f(x[m]));
        if (b)
            swap(f, t[u].f);
        if (r - l == 1)
            return;
        a != b ? put(u * 2, l, m, f) : put(u * 2 + 1, m, r, f);
    }
    void add0(int u, int l, int r, int ql, int qr, nline<T> f) {
        if (ql <= l && r <= qr)
            return put(u, l, r, f);
        int m = (l + r) / 2;
        if (ql < m)
            add0(u * 2, l, m, ql, qr, f);
        if (m < qr)
            add0(u * 2 + 1, m, r, ql, qr, f);
    }
    void add(nline<T> f) {
        if (!empty())
            put(1, 0, len(), f);
    }
    void addidx(nline<T> f, int l, int r) {
        nassert(0 <= l && l <= r && r <= len());
        if (l < r)
            add0(1, 0, len(), l, r, f);
    }
    void addseg(nline<T> f, T l, T r) { addidx(f, nlower(x, l), nlower(x, r)); }
    nmaybe<V> get(T v) const {
        int i = nlower(x, v);
        if (i == len() || v < x[i] || x[i] < v)
            return {};
        nmaybe<V> z;
        for (int u = 1, l = 0, r = len();;) {
            if (t[u].has) {
                V y = t[u].f(v);
                if (!z || cmp(y, z.val()))
                    z = y;
            }
            if (r - l == 1)
                break;
            int m = (l + r) / 2;
            if (i < m)
                r = m, u *= 2;
            else
                l = m, u = u * 2 + 1;
        }
        return z;
    }
    V get(T v, V d) const {
        auto z = get(v);
        return z ? z.val() : move(d);
    }
    V operator()(T v, V d) const { return get(v, move(d)); }
};

// 50 nio
struct ninput {
    static constexpr int S = 1 << 16;
    FILE* f = stdin;
    array<unsigned char, S> b{};
    int p = 0, n = 0;
    bool good = true;
    ninput() = default;
    explicit ninput(FILE* f) : f(f) {}
    int gc() {
        if (p == n)
            n = int(fread(b.data(), 1, S, f)), p = 0;
        return p < n ? b[p++] : EOF;
    }
    bool skip(int& c) {
        do
            c = gc();
        while (c != EOF && c <= 32);
        return c != EOF;
    }
    template <class T>
        requires(is_integral_v<T> || is_same_v<T, __int128_t> || is_same_v<T, __uint128_t>)
    bool read(T& x) {
        int c;
        if (!skip(c))
            return good = false;
        bool neg = c == '-';
        if (c == '+' || c == '-')
            c = gc();
        using U = conditional_t<is_same_v<T, __int128_t>, __uint128_t, make_unsigned_t<T>>;
        U z = 0;
        bool any = false;
        for (; c >= '0' && c <= '9'; c = gc())
            z = z * 10 + unsigned(c - '0'), any = true;
        if (!any)
            return good = false;
        if constexpr (is_unsigned_v<T> || is_same_v<T, __uint128_t>) {
            nassert(!neg);
            x = T(z);
        } else
            x = neg ? T(U(0) - z) : T(z);
        return true;
    }
    bool read(char& x) {
        int c;
        if (!skip(c))
            return good = false;
        x = char(c);
        return true;
    }
    bool read(string& x) {
        int c;
        if (!skip(c))
            return good = false;
        x.clear();
        do
            x.push_back(char(c)), c = gc();
        while (c != EOF && c > 32);
        return true;
    }
    template <floating_point T> bool read(T& x) {
        string s;
        if (!read(s))
            return false;
        x = T(strtold(s.c_str(), nullptr));
        return true;
    }
    template <class T>
        requires(!is_arithmetic_v<T> && constructible_from<T, long long>)
    bool read(T& x) {
        long long z;
        if (!read(z))
            return false;
        x = T(z);
        return true;
    }
    template <class A, class... B>
        requires(sizeof...(B) > 0)
    bool read(A& a, B&... b) {
        return read(a) && (read(b) && ...);
    }
    template <class T> nmaybe<T> next() {
        T x;
        return read(x) ? nmaybe<T>(move(x)) : nmaybe<T>{};
    }
    template <class T> T next(T d) {
        auto x = next<T>();
        return x ? move(x.val()) : move(d);
    }
    template <class T> ninput& operator>>(T& x) {
        read(x);
        return *this;
    }
    explicit operator bool() const { return good; }
};
struct noutput {
    static constexpr int S = 1 << 16;
    FILE* f = stdout;
    array<char, S> b{};
    int n = 0;
    noutput() = default;
    explicit noutput(FILE* f) : f(f) {}
    ~noutput() { flush(); }
    void flush() {
        if (n)
            fwrite(b.data(), 1, n, f), n = 0;
    }
    void pc(char c) {
        if (n == S)
            flush();
        b[n++] = c;
    }
    void write(char c) { pc(c); }
    void write(string_view s) {
        for (char c : s)
            pc(c);
    }
    void write(const string& s) { write(string_view(s)); }
    void write(const char* s) { write(string_view(s)); }
    template <class T>
        requires(is_integral_v<T> || is_same_v<T, __int128_t> || is_same_v<T, __uint128_t>)
    void write(T x) {
        using U = conditional_t<is_same_v<T, __int128_t>, __uint128_t, make_unsigned_t<T>>;
        U z;
        if constexpr (is_signed_v<T> || is_same_v<T, __int128_t>)
            if (x < 0)
                pc('-'), z = U(0) - U(x);
            else
                z = U(x);
        else
            z = U(x);
        char s[64];
        int k = 0;
        do
            s[k++] = char('0' + z % 10), z /= 10;
        while (z);
        while (k)
            pc(s[--k]);
    }
    template <floating_point T> void write(T x) {
        char s[64];
        int k = snprintf(s, sizeof s, "%.15Lg", static_cast<long double>(x));
        write(string_view(s, size_t(k)));
    }
    template <class T> noutput& operator<<(const T& x) {
        write(x);
        return *this;
    }
    void space() { pc(' '); }
    void line() { pc('\n'); }
};
inline ninput nin;
inline noutput nout;
template <class... A> bool nread(A&... a) {
    return nin.read(a...);
}
template <class A> void nprint_one(const A& a) {
    nout.write(a);
}
template <class A, class... B> void nprint(const A& a, const B&... b) {
    nprint_one(a);
    ((nout.space(), nprint_one(b)), ...);
}
template <class... A> void nprintln(const A&... a) {
    if constexpr (sizeof...(a))
        nprint(a...);
    nout.line();
}

// 99 ndefault
template <class T> using nvector = nvector_stl<T>;
template <class T> using ndeque = ndeque_ring<T>;
template <class T, class C = nless<T>> using nheap = nheap_binary<T, C>;
template <class T, class C = nless<T>, class A = nempty_augment<T>> using nset = nset_fhq<T, C, false, A>;
template <class T, class C = nless<T>, class A = nempty_augment<T>> using nbag = nset_fhq<T, C, true, A>;
template <class K, class V, class H = nhash<K>, class E = equal_to<K>> using nmap = nmap_flat<K, V, H, E>;
template <class A, class B, class HA = nhash<A>, class HB = nhash<B>, class EA = equal_to<A>, class EB = equal_to<B>>
using nbije = nbije_hash<A, B, HA, HB, EA, EB>;
template <class A, class B, class HA = nhash<A>, class HB = nhash<B>, class EA = equal_to<A>, class EB = equal_to<B>>
using ninj = nbije_hash<A, B, HA, HB, EA, EB>;
using npart = npart_dense;
template <uint64_t M> using nmod = nmod_static<M>;
template <int Tag = 0> using ndmod = nmod_dynamic<Tag>;
#define ngcd ngcd_binary
#define nisprime nisprime_miller
#define nfactor nfactor_rho
#define nconv nconv_auto
#define nscc nscc_kosaraju
#define nsg nsg_dag
template <class W = int> using ngraph = ngraph_forward<W>;
template <class T> using nflow = nflow_dinic<T>;
using nbimatch = nbimatch_hopcroft;
template <class T, class C = nless<nwide_t<T>>> using nlichao = nlichao_static<T, C>;
