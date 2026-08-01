#include <bits/stdc++.h>
using namespace std;

#if defined(NITORI_V2_CHECKED) == defined(NITORI_V2_UNSAFE)
#error "exactly one Nitori v2 profile must be selected"
#endif

namespace ni {
[[noreturn]] inline void ncontract_fail(const char* expr, const char* file, int line) noexcept {
    fprintf(stderr, "Nitori contract failed: %s (%s:%d)\n", expr, file, line);
    abort();
}
} // namespace ni

#if defined(NITORI_V2_UNSAFE)
#if defined(__GNUC__) || defined(__clang__)
#define npre(expr)                                                                                                    \
    do {                                                                                                              \
        if (!(expr))                                                                                                  \
            __builtin_unreachable();                                                                                  \
    } while (false)
#else
#define npre(expr) ((void)0)
#endif
#else
#define npre(expr)                                                                                                    \
    do {                                                                                                              \
        if (!(expr))                                                                                                  \
            ::ni::ncontract_fail(#expr, __FILE__, __LINE__);                                                          \
    } while (false)
#endif

#define nassert(expr) npre(expr)

inline constexpr int nversion = 20000;
#if defined(NITORI_V2_UNSAFE)
inline constexpr bool nunsafe = true;
#else
inline constexpr bool nunsafe = false;
#endif
