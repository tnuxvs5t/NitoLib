#pragma once

#ifdef NITORI_TEST_UNSAFE
#include "../v2_unsafe/Nitori.h"
#else
#include "../v2/Nitori.h"
#endif

#define ntest(expr)                                                                                                   \
    do {                                                                                                              \
        if (!(expr)) {                                                                                                \
            fprintf(stderr, "test failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__);                                \
            return 1;                                                                                                 \
        }                                                                                                             \
    } while (false)
