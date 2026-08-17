#pragma once
#include "core.hpp"

/* modulus is nonzero; operands are reduced internally. */
constexpr uint64_t naddmod64(uint64_t left, uint64_t right, uint64_t modulus) {
    uint64_t gap = modulus - right;
    return left >= gap ? left - gap : left + right;
}

constexpr uint64_t nmulmod64(uint64_t left, uint64_t right, uint64_t modulus) {
    left %= modulus;
    right %= modulus;
    if (modulus <= 3'037'000'499ULL) return left * right % modulus;
    uint64_t result = 0;
    while (right) {
        if (right & 1) result = naddmod64(result, left, modulus);
        right >>= 1;
        if (right) left = naddmod64(left, left, modulus);
    }
    return result;
}

constexpr uint64_t npowmod64(uint64_t base, uint64_t exponent, uint64_t modulus) {
    uint64_t result = 1 % modulus;
    while (exponent) {
        if (exponent & 1) result = nmulmod64(result, base, modulus);
        exponent >>= 1;
        if (exponent) base = nmulmod64(base, base, modulus);
    }
    return result;
}

/* Deterministic Miller-Rabin for every uint64_t. */
constexpr bool nisprime(uint64_t value) {
    if (value < 2) return false;
    for (uint64_t prime : {2ULL, 3ULL, 5ULL, 7ULL, 11ULL, 13ULL, 17ULL, 19ULL,
                           23ULL, 29ULL, 31ULL, 37ULL})
        if (value % prime == 0) return value == prime;
    uint64_t odd = value - 1;
    int power = countr_zero(odd);
    odd >>= power;
    for (uint64_t witness : {2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL,
                             9780504ULL, 1795265022ULL}) {
        if (witness % value == 0) continue;
        uint64_t current = npowmod64(witness % value, odd, value);
        if (current == 1 || current == value - 1) continue;
        bool passed = false;
        for (int round = 1; round < power; ++round) {
            current = nmulmod64(current, current, value);
            if (current == value - 1) { passed = true; break; }
        }
        if (!passed) return false;
    }
    return true;
}

inline uint64_t nsplitmix64(uint64_t& state) {
    uint64_t value = (state += 0x9e3779b97f4a7c15ULL);
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

/* value is odd composite.  Expected running time is roughly O(value^(1/4)). */
inline uint64_t npollard(uint64_t value, uint64_t seed = 0x243f6a8885a308d3ULL) {
    if (value % 2 == 0) return 2;
    if (value % 3 == 0) return 3;
    uint64_t state = seed ^ value;
    auto difference = [](uint64_t a, uint64_t b) { return a > b ? a - b : b - a; };
    while (true) {
        uint64_t y = nsplitmix64(state) % (value - 1) + 1;
        uint64_t constant = nsplitmix64(state) % (value - 1) + 1;
        auto advance = [&](uint64_t x) {
            return naddmod64(nmulmod64(x, x, value), constant, value);
        };
        uint64_t divisor = 1, radius = 1, x = 0, saved = 0;
        while (divisor == 1) {
            x = y;
            for (uint64_t i = 0; i < radius; ++i) y = advance(y);
            for (uint64_t done = 0; done < radius && divisor == 1; done += 128) {
                saved = y;
                uint64_t product = 1, block = min<uint64_t>(128, radius - done);
                for (uint64_t i = 0; i < block; ++i) {
                    y = advance(y);
                    product = nmulmod64(product, difference(x, y), value);
                }
                divisor = gcd(product, value);
            }
            radius <<= 1;
        }
        if (divisor == value) {
            do {
                saved = advance(saved);
                divisor = gcd(difference(x, saved), value);
            } while (divisor == 1);
        }
        if (divisor != value) return divisor;
    }
}

/* value >= 1; returns prime factors with multiplicity in ascending order. */
inline vector<uint64_t> nfactor(uint64_t value) {
    vector<uint64_t> pending{value}, result;
    uint64_t seed = 0x13198a2e03707344ULL;
    while (!pending.empty()) {
        uint64_t current = pending.back();
        pending.pop_back();
        if (current == 1) continue;
        if (nisprime(current)) {
            result.push_back(current);
            continue;
        }
        uint64_t divisor = npollard(current, nsplitmix64(seed));
        pending.push_back(divisor);
        pending.push_back(current / divisor);
    }
    sort(result.begin(), result.end());
    return result;
}
