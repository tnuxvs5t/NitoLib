#include "../src-v3/arena.hpp"
#include "../src-v3/discrete.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

template <class T>
concept can_nrange = requires(T value) { nrange(value, value); };

int main() {
    static_assert(same_as<decltype(nlen(declval<vector<long long>&>())), nidx_t>);
    static_assert(same_as<decltype(nrange(3).len()), nidx_t>);
    static_assert(same_as<typename decltype(nrange(3))::iterator::difference_type, nidx_t>);
    static_assert(same_as<decltype(declval<narena<long long>&>().make(0)), nidx_t>);

#ifdef NITORI_INDEX_64
    static_assert(same_as<nidx_t, long long>);
    static_assert(can_nrange<long long>);

    constexpr long long X = 1'000'000'000'000'000'000LL;
    vector<long long> bucket(40000);
    auto dp = nanchors(nrange(X - 20000, X + 20000), nall(bucket));
    dp(X) = 7;
    CHECK(bucket[20000] == 7 && dp.key(20000) == X);

    auto huge = nrange(0LL, 5'000'000'000LL);
    CHECK(huge.len() == 5'000'000'000LL);
    CHECK(huge[4'999'999'999LL] == 4'999'999'999LL);

    auto cells = nproduct(nrange(50000LL), nrange(50000LL));
    CHECK(cells.len() == 2'500'000'000LL);
    CHECK((cells[2'499'999'999LL] == pair{49999LL, 49999LL}));

    auto stride = nstride(huge, 4'000'000'000LL, 5'000'000'000LL, 3LL);
    CHECK(stride.len() == 333'333'334LL);
    CHECK(stride[0] == 4'000'000'000LL);
    CHECK(stride[stride.len() - 1] == 4'999'999'999LL);
#else
    static_assert(same_as<nidx_t, int>);
    static_assert(!can_nrange<long long>);

    auto small = nrange(-10, 10);
    CHECK(small.len() == 20 && small[19] == 9 && small.inverse(0) == 10);
#endif

    vector<long long> keys{11, 17, 23}, values{1, 2, 3};
    auto function = nanchors(nall(keys), nall(values));
    static_assert(same_as<decltype(function(11)), long long&>);
    CHECK(function(23) == 3);
}
