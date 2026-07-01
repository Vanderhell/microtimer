#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_failures = 0;
static int test_assertions = 0;

#define TEST_ASSERT(expr)                                                        \
    do {                                                                         \
        ++test_assertions;                                                       \
        if (!(expr)) {                                                           \
            ++test_failures;                                                     \
            fprintf(stderr, "%s:%d: assertion failed: %s\n", __FILE__, __LINE__, \
                    #expr);                                                      \
            return 1;                                                            \
        }                                                                        \
    } while (0)

#define TEST_ASSERT_EQ(expected, actual) TEST_ASSERT((expected) == (actual))
#define TEST_ASSERT_STR_EQ(expected, actual) TEST_ASSERT(strcmp((expected), (actual)) == 0)

typedef int (*test_fn)(void);

static int run_test(const char *name, test_fn fn)
{
    int rc = fn();

    if (rc == 0) {
        printf("[PASS] %s\n", name);
    } else {
        printf("[FAIL] %s\n", name);
    }

    return rc;
}

#endif
