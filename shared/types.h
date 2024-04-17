#pragma once

#include <stdint.h>

#define CGLM_DEFINE_PRINTS

#include <cglm/cglm.h>

#define array_length(array) ((size_t)sizeof(array) / (size_t)sizeof(array[0]))

#define NONULL(...) __attribute__((nonnull(__VA_ARGS__)))

typedef enum Result
{
    RS_SUCCESS = 0,
    RS_FAILURE,
} Result;

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
