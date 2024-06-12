#pragma once
#include <stdint.h>
#include <stdlib.h>

#define array_create(array, array_capacity)                                    \
    do                                                                         \
    {                                                                          \
        (array)->size = 0;                                                     \
        (array)->capacity = (array_capacity);                                  \
        (array)->data = calloc(array_capacity, sizeof((*(array)->data)));      \
    } while (0)

#define array_push(array, value)                                               \
    do                                                                         \
    {                                                                          \
        if ((array)->size >= (array)->capacity)                                \
        {                                                                      \
            (array)->capacity *= 2;                                            \
            (array)->data = realloc(                                           \
                (array)->data, (array)->capacity * sizeof((*(array)->data)));  \
        }                                                                      \
        (array)->data[(array)->size++] = (value);                              \
    } while (0)

#define array_back(array) ((array)->data + ((array)->size - 1))

#define ftic_assert(ex)                                                        \
    if (!(ex)) *(u32*)0 = 0

#define true 1
#define false 0

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef int64_t b64;
typedef int32_t b32;
typedef int16_t b16;
typedef int8_t b8;

typedef double f64;
typedef float f32;

#define global static
#define internal static
#define presist static
