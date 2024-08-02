#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define PI 3.141592653589f
#define KILOBYTE(n) ((n) * 1024ULL)
#define MEGABYTE(n) (KILOBYTE((n)) * 1024ULL)
#define GIGABYTE(n) (MEGABYTE((n)) * 1024ULL)

#define MILLISECONDS(milli) ((milli) * 0.001);
#define MICROSECONDS(micro) ((micro) * 0.000001);
#define NANOSECONDS(nano) ((nano) * 0.000000001);

#define sysprintf(...) sprintf_s(__VA_ARGS__)
#define syscanf(...) sscanf_s(__VA_ARGS__)
#define sy_gcvt(...) _gcvt_s(__VA_ARGS__);

#define value_to_string(buffer, ...) sysprintf(buffer, sizeof((buffer)), __VA_ARGS__)

#define value_to_string_offset(buffer, offset, ...)                                                \
    sysprintf((buffer) + (offset), sizeof((buffer)) - (offset), __VA_ARGS__)

#define string_to_value(buffer, ...) syscanf((buffer), __VA_ARGS__)

#define f32_to_string(buffer, num_digits, val) sy_gcvt(buffer, sizeof((buffer)), val, num_digits)

#define f32_to_string_offset(buffer, offset, num_digits, val)                                      \
    sy_gcvt((buffer) + (offset), sizeof((buffer)) - (offset), val, num_digits)

#define ArraySigniture(name, type)                                                                 \
    typedef struct name##Array                                                                     \
    {                                                                                              \
        u32 size;                                                                                  \
        u32 capacity;                                                                              \
        type* data;                                                                                \
    } name##Array;

#define array_create(array, array_capacity)                                                        \
    do                                                                                             \
    {                                                                                              \
        (array)->size = 0;                                                                         \
        (array)->capacity = max((array_capacity), 2);                                              \
        (array)->data = calloc(array_capacity, sizeof((*(array)->data)));                          \
    } while (0)

#define array_push(array, value)                                                                   \
    do                                                                                             \
    {                                                                                              \
        if ((array)->size >= (array)->capacity)                                                    \
        {                                                                                          \
            (array)->capacity = (u32)(1.5f * (array)->capacity);                                   \
            (array)->data = realloc((array)->data, (array)->capacity * sizeof((*(array)->data)));  \
        }                                                                                          \
        (array)->data[(array)->size++] = (value);                                                  \
    } while (0)

#define array_back(array) ((array)->data + ((array)->size - 1))

#define array_free(array) free((array)->data)

#define safe_array_push(arr, value)                                                                \
    do                                                                                             \
    {                                                                                              \
        platform_mutex_lock(&(arr)->mutex);                                                        \
        array_push(&(arr)->array, value);                                                          \
        platform_mutex_unlock(&(arr)->mutex);                                                      \
    } while (0)

#define safe_array_create(arr, array_capacity)                                                     \
    do                                                                                             \
    {                                                                                              \
        array_create(&(arr)->array, array_capacity);                                               \
        (arr)->mutex = platform_mutex_create();                                                    \
    } while (0)

#define safe_array_free(arr)                                                                       \
    do                                                                                             \
    {                                                                                              \
        array_free(&(arr)->array);                                                                 \
        platform_mutex_destroy(&(arr)->mutex);                                                     \
    } while (0)

#define array_move_to_front(type, array, index)                                                    \
    do                                                                                             \
    {                                                                                              \
        type temp = (array)->data[(index)];                                                        \
        for (i32 i34251 = (index); i34251 >= 1; --i34251)                                          \
        {                                                                                          \
            (array)->data[i34251] = (array)->data[i34251 - 1];                                     \
        }                                                                                          \
        (array)->data[0] = temp;                                                                   \
    } while (0)

#define ftic_max(first, second) ((first) + (((second) - (first)) * ((second) > (first))))
#define ftic_min(first, second) ((first) + (((second) - (first)) * ((second) < (first))))
#define ftic_clamp_low(value, low) ftic_max((value), (low))
#define ftic_clamp_high(value, high) ftic_min((value), (high))

#define static_array_size(array) (sizeof(array) / sizeof(array[0]))

#define thread_return_value unsigned long
#define FTicThreadHandle void*
#define FTicMutex void*
#define FTicSemaphore void*

#define ftic_assert(ex)                                                                            \
    if (!(ex)) *(u32*)0 = 0

#define b_switch(val) (val) = (val) ? false : true
#define closed_interval(low, val, high) ((val) >= (low) && (val) <= (high))
#define open_interval(low, val, high) ((val) > (low) && (val) < (high))

#define true 1
#define false 0

#define FTIC_MAX_PATH 260

#define set_bit(val, bit) (val) |= (bit)
#define unset_bit(val, bit) (val) &= ~(bit)
#define switch_bit(val, bit) (val) ^= (bit)
#define check_bit(val, bit) (((val) & (bit)) == (bit))

#define BIT_64 0x8000000000000000
#define BIT_63 0x4000000000000000
#define BIT_62 0x2000000000000000
#define BIT_61 0x1000000000000000
#define BIT_60 0x800000000000000
#define BIT_59 0x400000000000000
#define BIT_58 0x200000000000000
#define BIT_57 0x100000000000000
#define BIT_56 0x80000000000000
#define BIT_55 0x40000000000000
#define BIT_54 0x20000000000000
#define BIT_53 0x10000000000000
#define BIT_52 0x8000000000000
#define BIT_51 0x4000000000000
#define BIT_50 0x2000000000000
#define BIT_49 0x1000000000000
#define BIT_48 0x800000000000
#define BIT_47 0x400000000000
#define BIT_46 0x200000000000
#define BIT_45 0x100000000000
#define BIT_44 0x80000000000
#define BIT_43 0x40000000000
#define BIT_42 0x20000000000
#define BIT_41 0x10000000000
#define BIT_40 0x8000000000
#define BIT_39 0x4000000000
#define BIT_38 0x2000000000
#define BIT_37 0x1000000000
#define BIT_36 0x800000000
#define BIT_35 0x400000000
#define BIT_34 0x200000000
#define BIT_33 0x100000000
#define BIT_32 0x80000000
#define BIT_31 0x40000000
#define BIT_30 0x20000000
#define BIT_29 0x10000000
#define BIT_28 0x8000000
#define BIT_27 0x4000000
#define BIT_26 0x2000000
#define BIT_25 0x1000000
#define BIT_24 0x800000
#define BIT_23 0x400000
#define BIT_22 0x200000
#define BIT_21 0x100000
#define BIT_20 0x80000
#define BIT_19 0x40000
#define BIT_18 0x20000
#define BIT_17 0x10000
#define BIT_16 0x8000
#define BIT_15 0x4000
#define BIT_14 0x2000
#define BIT_13 0x1000
#define BIT_12 0x800
#define BIT_11 0x400
#define BIT_10 0x200
#define BIT_9 0x100
#define BIT_8 0x80
#define BIT_7 0x40
#define BIT_6 0x20
#define BIT_5 0x10
#define BIT_4 0x8
#define BIT_3 0x4
#define BIT_2 0x2
#define BIT_1 0x1

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

