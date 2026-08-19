#pragma once
#include "view.hpp"
#include <atomic>

namespace nhash_detail {
constexpr uint64_t mix(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

inline uint64_t seed() {
    static atomic<uint64_t> sequence{0x9e3779b97f4a7c15ULL};
    uint64_t time = uint64_t(chrono::steady_clock::now().time_since_epoch().count());
    return mix(time ^ sequence.fetch_add(0x9e3779b97f4a7c15ULL, memory_order_relaxed));
}

template <class T> uint64_t value(const T& object, uint64_t salt);

template <class A, class B> uint64_t value(const pair<A, B>& object, uint64_t salt);

template <class... A> uint64_t value(const tuple<A...>& object, uint64_t salt);

inline uint64_t value(const char* object, uint64_t salt) {
    return mix(uint64_t(hash<string_view>{}(object)) ^ salt);
}

template <size_t N>
uint64_t value(const char (&object)[N], uint64_t salt) {
    return mix(uint64_t(hash<string_view>{}(string_view(object, N - 1))) ^ salt);
}

template <class A, class B> uint64_t value(const pair<A, B>& object, uint64_t salt) {
    uint64_t first = value(object.first, salt ^ 0x243f6a8885a308d3ULL);
    uint64_t second = value(object.second, salt ^ 0x13198a2e03707344ULL);
    return mix(salt ^ 0x243f6a8885a308d3ULL ^ first ^ (rotl(second, 29) + 0x9e3779b97f4a7c15ULL));
}

template <class T, size_t... I> uint64_t tuple_value(const T& object, uint64_t salt, index_sequence<I...>) {
    uint64_t result = salt ^ 0x6a09e667f3bcc909ULL ^ sizeof...(I);
    ((result = result * 0x9e3779b97f4a7c15ULL + value(get<I>(object), salt + 0x9e3779b97f4a7c15ULL * (I + 1)) + I),
     ...);
    return mix(result);
}

template <class... A> uint64_t value(const tuple<A...>& object, uint64_t salt) {
    return tuple_value(object, salt, index_sequence_for<A...>{});
}

template <class T> uint64_t value(const T& object, uint64_t salt) {
    using U = remove_cvref_t<T>;
    return mix(uint64_t(hash<U>{}(object)) ^ salt);
}
} // namespace nhash_detail

/* A salted structural hash: std::hash for leaves, recursive order-sensitive mixing for pair/tuple. */
struct nhash {
  private:
    uint64_t salt;

  public:
    nhash() : salt(nhash_detail::seed()) {}
    explicit nhash(uint64_t fixed_salt) : salt(fixed_salt) {}

    template <class T> size_t operator()(const T& object) const { return size_t(nhash_detail::value(object, salt)); }
};

/*
Static unique key -> source position inverse.  Keys are stored densely and each slot
contains a 32-bit fingerprint plus an nidx_t position; a negative position is empty.
Construction knows the
final size, so there is no growth, erase, tombstone, node allocation or rehash.  Hash and
equality agree, queried keys are stable, and positions fit nidx_t.  A missing key
returns -1.  Build is expected O(n), lookup expected O(1), storage O(n).
*/
template <class K, class H = nhash, class E = equal_to<>>
struct nhash_inverse {
  private:
    struct slot {
        uint32_t fingerprint = 0;
        nidx_t position = -1;
    };

    vector<K> keys;
    vector<slot> table;
    size_t mask = 0;
    H hasher;
    E equal;

    static size_t capacity_for(nidx_t expected) {
        if (expected <= 0)
            return 1;
        size_t need = size_t(expected);
        size_t required = (need * 5 + 3) / 4;
        size_t capacity = 1;
        while (capacity < required)
            capacity <<= 1;
        return capacity;
    }

    static uint32_t tag(uint64_t hash) { return uint32_t(hash >> 32); }

  public:
    template <class V>
    explicit nhash_inverse(const V& source, H hash = {}, E relation = {})
        : hasher(move(hash)), equal(move(relation)) {
        nidx_t n = source.len();
        keys.reserve(n);
        for (nidx_t i = 0; i < n; ++i)
            keys.push_back(nview_detail::own(source[i]));
        table.assign(capacity_for(n), {});
        mask = table.size() - 1;
        for (nidx_t i = 0; i < n; ++i) {
            uint64_t hash_value = uint64_t(hasher(keys[i]));
            size_t at = hash_value & mask;
            while (table[at].position >= 0)
                at = (at + 1) & mask;
            table[at] = {tag(hash_value), i};
        }
    }

    static constexpr size_t storage_key_bytes() { return sizeof(K); }
    static constexpr size_t storage_slot_bytes() { return sizeof(slot); }

    template <class Q>
    nidx_t find(const Q& key) const {
        uint64_t hash = uint64_t(hasher(key));
        size_t at = hash & mask;
        while (table[at].position >= 0) {
            const slot& cell = table[at];
            if (cell.fingerprint == tag(hash) && equal(keys[cell.position], key))
                return cell.position;
            at = (at + 1) & mask;
        }
        return -1;
    }
};

template <class V, class H = nhash, class E = equal_to<>>
auto nmake_hash_inverse(const V& view, H hash = {}, E equal = {}) {
    using K = nview_detail::owned_t<decltype(view[0])>;
    return nhash_inverse<K, H, E>(view, move(hash), move(equal));
}

namespace nhash_detail {
template <class V, class I>
struct inverted_access {
    V view;
    I locate;

    constexpr decltype(auto) operator()(nidx_t position) { return view[position]; }

    template <class K>
    nidx_t inverse(const K& key) const { return locate.find(key); }
};
}

/* Preserve an existing structural inverse; otherwise attach one static hash fallback. */
template <class V>
requires requires(V& view) { view.inverse(view[0]); }
constexpr V ninvert(V view) {
    return view;
}

template <class V, class H, class E>
auto ninvert(V view, H hash, E equal) {
    nidx_t n = view.len();
    auto locate = nmake_hash_inverse(view, move(hash), move(equal));
    using I = decltype(locate);
    return nview{n, nhash_detail::inverted_access<V, I>{move(view), move(locate)}};
}

template <class V>
requires (!requires(V& view) { view.inverse(view[0]); })
auto ninvert(V view) {
    return ninvert(move(view), nhash{}, equal_to<>{});
}
