#include "common.hpp"

template <class T>
concept nhas_extgcd = requires(T value) { nextgcd(value, value); };

int main() {
    static_assert(ninteger<int> && ninteger<__int128_t> && !ninteger<bool>);
    static_assert(nhas_extgcd<long long> && !nhas_extgcd<__int128_t>);

    ntest(nmag(LLONG_MIN) == (1ULL << 63));
    ntest(ngcd(84, 30) == 6 && ngcd(LLONG_MIN, 0LL) == (1ULL << 63));
    ntest(nlcm(12, -18) == 36);
    ntest(nfloor_div(-7, 3) == -3 && nceil_div(-7, 3) == -2);
    ntest(nfloor_div(7, -3) == -3 && nceil_div(7, -3) == -2);
    ntest(nmodulo(-17, 5) == 3);

    auto bezout = nextgcd(-84LL, 30LL);
    ntest(bezout.gcd == 6);
    ntest(__int128_t(-84) * bezout.x + __int128_t(30) * bezout.y == 6);

    ntest(nmulmod(UINT64_MAX - 1, UINT64_MAX - 2, UINT64_MAX) == 2);
    ntest(npowmod(2, 100, 1000000007) == 976371285);

    for (uint64_t prime : {2ULL, 3ULL, 5ULL, 97ULL, 1000000007ULL, 2305843009213693951ULL})
        ntest(nisprime(prime));
    for (uint64_t composite : {0ULL, 1ULL, 4ULL, 91ULL, 1000000007ULL * 1000000009ULL})
        ntest(!nisprime(composite));

    ntest((nprimes(30) == nvector<int>{2, 3, 5, 7, 11, 13, 17, 19, 23, 29}));

    nseed(0x5001U);
    uint64_t semiprime = 1000000007ULL * 1000000009ULL;
    ntest(nfactor(semiprime) == nvector<uint64_t>({1000000007ULL, 1000000009ULL}));
    ntest(nfactor(1).empty());

    nfrac<long long> fraction(-8, -12);
    ntest(fraction == nfrac<long long>(2, 3));
    ntest(fraction + nfrac<long long>(5, 6) == nfrac<long long>(3, 2));
    ntest(fraction * nfrac<long long>(9, 4) == nfrac<long long>(3, 2));
    ntest(nfrac<long long>(-7, 3).floor() == -3);
    ntest(nfrac<long long>(-7, 3).ceil() == -2);
    ntest(!fraction.trydiv(nfrac<long long>(0)));

    auto congruence = ncrt(ncongruence(2, 6), ncongruence(5, 9));
    ntest(congruence && congruence->a == 14 && congruence->m == 18);
    ntest(congruence->has(50) && !congruence->has(51));
    ntest(!ncrt(ncongruence(0, 4), ncongruence(1, 2)));

    nprime_table table(100);
    ntest(table.p == nprimes(100));
    ntest(table.isprime(97) && !table.isprime(91));
    ntest(table.phi[60] == 16 && table.mu[60] == 0);
    ntest((table.factor(84) == nvector<pair<int, int>>({{2, 2}, {3, 1}, {7, 1}})));
}
