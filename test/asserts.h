#pragma once
#include <stdio.h>

#define ASSERT_EQUALS(expected, actually)                                      \
    do                                                                         \
    {                                                                          \
        if (expected != actually)                                              \
        {                                                                      \
            printf("%s:%d\n", __FILE__, __LINE__);                             \
        }                                                                      \
    } while (0)

#define ASSERT_TRUE(expected)                                                  \
    do                                                                         \
    {                                                                          \
        if (!expected)                                                         \
        {                                                                      \
            printf("%s:%d\n", __FILE__, __LINE__);                             \
        }                                                                      \
    } while (0)

#define ASSERT_TRUE_MSG(expected, msg)                                         \
    do                                                                         \
    {                                                                          \
        if (!expected)                                                         \
        {                                                                      \
            printf("%s:%d\n%s\n", __FILE__, __LINE__, msg);                      \
        }                                                                      \
    } while (0)

#define ASSERT_FALSE(expected) ASSERT_TRUE(!expected)

#define ASSERT_FALSE_MSG(expected, msg) ASSERT_TRUE_MSG(!expected, msg)
