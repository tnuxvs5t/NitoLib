#include "common.hpp"

int main() {
    mt19937_64 rng(33166247);
    for (int trial = 0; trial < 200000; ++trial) {
        long long a = static_cast<long long>(rng());
        long long b = static_cast<long long>(rng() | 1ULL);
        if (a == LLONG_MIN && b == -1)
            continue;
        long long floor = nfloor_div(a, b), ceil = nceil_div(a, b);
        __int128 aa = a, bb = b;
        __int128 floor_reference = aa / bb, floor_remainder = aa % bb;
        if (floor_remainder && ((floor_remainder < 0) != (bb < 0)))
            --floor_reference;
        __int128 ceil_reference = aa / bb, ceil_remainder = aa % bb;
        if (ceil_remainder && ((ceil_remainder < 0) == (bb < 0)))
            ++ceil_reference;
        ntest(floor == floor_reference && ceil == ceil_reference);

        auto result = nextgcd(a, b);
        ntest(__int128(a) * result.x + __int128(b) * result.y == result.gcd);
        ntest(result.gcd == ngcd(a, b));
    }

    vector<bool> prime(200000, true);
    prime[0] = prime[1] = false;
    for (int value = 2; value * value < int(prime.size()); ++value)
        if (prime[value])
            for (int multiple = value * value; multiple < int(prime.size()); multiple += value)
                prime[multiple] = false;
    for (int value = 0; value < int(prime.size()); ++value)
        ntest(nisprime(value) == prime[value]);
}
