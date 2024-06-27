#pragma once
#include <stdint.h>
#include <stdlib.h>

#define PI 3.141592653589f


#define array_create(array, array_capacity)                                    \
    do                                                                         \
    {                                                                          \
        (array)->size = 0;                                                     \
        (array)->capacity = ((array_capacity) == 1 ? 2 : (array_capacity));        \
        (array)->data = calloc(array_capacity, sizeof((*(array)->data)));      \
    } while (0)

#define array_push(array, value)                                               \
    do                                                                         \
    {                                                                          \
        if ((array)->size >= (array)->capacity)                                \
        {                                                                      \
            (array)->capacity = (u32)(1.5f * (array)->capacity);               \
            (array)->data = realloc(                                           \
                (array)->data, (array)->capacity * sizeof((*(array)->data)));  \
        }                                                                      \
        (array)->data[(array)->size++] = (value);                              \
    } while (0)

#define array_back(array) ((array)->data + ((array)->size - 1))

#define safe_array_push(arr, value)                                            \
    do                                                                         \
    {                                                                          \
        platform_mutex_lock(&(arr)->mutex);                                    \
        array_push(&(arr)->array, value);                                      \
        platform_mutex_unlock(&(arr)->mutex);                                  \
    } while (0)

#define safe_array_create(arr, array_capacity)                                 \
    do                                                                         \
    {                                                                          \
        array_create(&(arr)->array, array_capacity);                           \
        (arr)->mutex = platform_mutex_create();                                \
    } while (0)

#define static_array_size(array) (sizeof(array) / sizeof(array[0]))

#define thread_return_value unsigned long
#define FTicThreadHandle void*
#define FTicMutex void*
#define FTicSemaphore void*

#define ftic_assert(ex)                                                        \
    if (!(ex)) *(u32*)0 = 0

#define b_switch(val) (val) = (val) ? false : true
#define closed_interval(low, val, high) ((val) >= (low) && (val) <= (high))
#define open_interval(low, val, high) ((val) > (low) && (val) < (high))

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

typedef struct CharPtrArray
{
    u32 size;
    u32 capacity;
    char** data;
} CharPtrArray;
