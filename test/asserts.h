#pragma once
#include <stdio.h>

#define EQUALS_FORMAT_FLOAT "Expected: %f, Actual: %f\n"
#define EQUALS_FORMAT_U32 "Expected: %u, Actual: %u\n"

#define ASSERT_EQUALS_WITHIN(expected, actually, margin, format)               \
    do                                                                         \
    {                                                                          \
        if (!closed_interval((-margin), (expected) - (actually), (margin)))    \
        {                                                                      \
            printf("%s:%d: \n\t", __FILE__, __LINE__);                         \
            printf(format, expected, actually);                                \
            ++g_total_test_failed_count;                                       \
        }                                                                      \
    } while (0)

#define ASSERT_EQUALS(expected, actually, format)                              \
    do                                                                         \
    {                                                                          \
        if ((expected) != (actually))                                          \
        {                                                                      \
            printf("%s:%d: \n\t", __FILE__, __LINE__);                         \
            printf(format, expected, actually);                                \
            printf("\n");                                                      \
            ++g_total_test_failed_count;                                       \
        }                                                                      \
    } while (0)

#define ASSERT_TRUE(expected)                                                  \
    do                                                                         \
    {                                                                          \
        if (!(expected))                                                       \
        {                                                                      \
            printf("%s:%d: \n", __FILE__, __LINE__);                           \
            printf("\n");                                                      \
            ++g_total_test_failed_count;                                       \
        }                                                                      \
    } while (0)

#define ASSERT_TRUE_MSG(expected, msg)                                         \
    do                                                                         \
    {                                                                          \
        if (!(expected))                                                       \
        {                                                                      \
            printf("%s:%d: \n%s\n", __FILE__, __LINE__, msg);                  \
            printf("\n");                                                      \
            ++g_total_test_failed_count;                                       \
        }                                                                      \
    } while (0)

#define ASSERT_FALSE(expected) ASSERT_TRUE(!(expected))

#define ASSERT_FALSE_MSG(expected, msg) ASSERT_TRUE_MSG(!(expected), (msg))
