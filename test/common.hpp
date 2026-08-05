#pragma once

#ifdef NITORI_X_TEST_UNSAFE
#include "../Nitori_unsafe.h"
#else
#include "../Nitori.h"
#endif

#define ntest(expr)                                                                                                   \
    do {                                                                                                              \
        if (!(expr)) {                                                                                                \
            fprintf(stderr, "test failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__);                                \
            return 1;                                                                                                 \
        }                                                                                                             \
    } while (false)
