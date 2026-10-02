#pragma once
#include "view.hpp"

namespace ndetail {

constexpr uint64_t nhash_mix(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

inline uint64_t nhash_seed() {
    static atomic<uint64_t> sequence{0x9e3779b97f4a7c15ULL};
    uint64_t now = uint64_t(chrono::steady_clock::now().time_since_epoch().count());
    return nhash_mix(now ^ sequence.fetch_add(0x9e3779b97f4a7c15ULL,
                                                memory_order_relaxed));
}

template <class T>
uint64_t nhash_value(const T& object, uint64_t salt);

template <class A, class B>
uint64_t nhash_value(const pair<A, B>& object, uint64_t salt);

template <class... A>
uint64_t nhash_value(const tuple<A...>& object, uint64_t salt);

inline uint64_t nhash_value(const char* object, uint64_t salt) {
    return nhash_mix(uint64_t(hash<string_view>{}(object)) ^ salt);
}

template <size_t N>
uint64_t nhash_value(const char (&object)[N], uint64_t salt) {
    return nhash_mix(uint64_t(hash<string_view>{}(string_view(object, N - 1))) ^ salt);
}

template <class A, class B>
uint64_t nhash_value(const pair<A, B>& object, uint64_t salt) {
    uint64_t first = nhash_value(object.first, salt ^ 0x243f6a8885a308d3ULL);
    uint64_t second = nhash_value(object.second, salt ^ 0x13198a2e03707344ULL);
    return nhash_mix(salt ^ 0x243f6a8885a308d3ULL ^ first ^
                     (rotl(second, 29) + 0x9e3779b97f4a7c15ULL));
}

template <class T, size_t... I>
uint64_t nhash_tuple_value(const T& object, uint64_t salt, index_sequence<I...>) {
    uint64_t result = salt ^ 0x6a09e667f3bcc909ULL ^ sizeof...(I);
    ((result = result * 0x9e3779b97f4a7c15ULL +
               nhash_value(get<I>(object), salt +
                           0x9e3779b97f4a7c15ULL * (I + 1)) + I), ...);
    return nhash_mix(result);
}

template <class... A>
uint64_t nhash_value(const tuple<A...>& object, uint64_t salt) {
    return nhash_tuple_value(object, salt, index_sequence_for<A...>{});
}

template <class T>
uint64_t nhash_value(const T& object, uint64_t salt) {
    using U = remove_cvref_t<T>;
    return nhash_mix(uint64_t(hash<U>{}(object)) ^ salt);
}

} // namespace ndetail

struct nhash {
    uint64_t salt;

    nhash() : salt(ndetail::nhash_seed()) {}
    explicit nhash(uint64_t fixed_salt) : salt(fixed_salt) {}

    template <class T>
    size_t operator()(const T& object) const {
        return size_t(ndetail::nhash_value(object, salt));
    }
};

/* Fixed-capacity open addressing: build O(n), expected lookup O(1), storage O(n). */
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
        if (expected <= 0) return 1;
        size_t required = (size_t(expected) * 5 + 3) / 4;
        size_t capacity = 1;
        while (capacity < required) capacity <<= 1;
        return capacity;
    }

    static uint32_t fingerprint(uint64_t value) {
        return uint32_t(value >> 32);
    }

public:
    template <class V>
    explicit nhash_inverse(const V& source, H hash = {}, E relation = {})
        : hasher(move(hash)), equal(move(relation)) {
        nidx_t n = source.len();
        keys.reserve(n);
        for (nidx_t i = 0; i < n; ++i)
            keys.push_back(nown(source[i]));
        table.assign(capacity_for(n), {});
        mask = table.size() - 1;
        for (nidx_t i = 0; i < n; ++i) {
            uint64_t value = uint64_t(hasher(keys[i]));
            size_t at = value & mask;
            while (table[at].position >= 0) at = (at + 1) & mask;
            table[at] = {fingerprint(value), i};
        }
    }

    static constexpr size_t storage_key_bytes() { return sizeof(K); }
    static constexpr size_t storage_slot_bytes() { return sizeof(slot); }

    template <class Q>
    nidx_t find(const Q& key) const {
        uint64_t value = uint64_t(hasher(key));
        size_t at = value & mask;
        while (table[at].position >= 0) {
            const slot& cell = table[at];
            if (cell.fingerprint == fingerprint(value) &&
                equal(keys[cell.position], key))
                return cell.position;
            at = (at + 1) & mask;
        }
        return -1;
    }
};

template <class V, class H = nhash, class E = equal_to<>>
auto nmake_hash_inverse(const V& view, H hash = {}, E equal = {}) {
    using K = nowned_t<decltype(view[0])>;
    return nhash_inverse<K, H, E>(view, move(hash), move(equal));
}

namespace ndetail {

template <class V, class I>
struct ninvert_access {
    V view;
    I locate;

    constexpr decltype(auto) operator()(nidx_t i) { return view[i]; }

    template <class K>
    nidx_t inverse(const K& key) const { return locate.find(key); }
};

} // namespace ndetail

template <class V>
requires requires(V& view) { view.inverse(view[0]); }
constexpr V ninvert(V view) {
    return view;
}

template <class V, class H, class E>
auto ninvert(V view, H hash, E equal) {
    nidx_t n = view.len();
    auto locate = nmake_hash_inverse(view, move(hash), move(equal));
    return nview{n, ndetail::ninvert_access<V, decltype(locate)>{move(view), move(locate)}};
}

template <class V>
requires (!requires(V& view) { view.inverse(view[0]); })
auto ninvert(V view) {
    return ninvert(move(view), nhash{}, equal_to<>{});
}
