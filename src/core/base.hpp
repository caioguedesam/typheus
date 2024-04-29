// ========================================================
// BASE
// Common codebase for all typheus modules.
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "stdafx.hpp"

namespace ty
{

// ========================================================
// [BASE TYPES]
typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef float       f32;
typedef double      f64;

typedef unsigned char byte;

typedef u32 handle;

// ========================================================
// [GENERAL DEFINES AND MACROS]

#define MAX_U8  (0xFF)
#define MAX_U16 (0xFFFF)
#define MAX_U32 (0xFFFFFFFFUL)
#define MAX_U64 (0xFFFFFFFFFFFFFFFFULL)

#define EPSILON_F32 FLT_EPSILON
#define EPSILON_F64 DBL_EPSILON

#undef KB
#undef MB
#undef GB
#define KB(V) ((V)   * 1024ULL)
#define MB(V) (KB(V) * 1024ULL)
#define GB(V) (MB(V) * 1024ULL)

#undef MIN
#undef MAX
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define CLAMP(V, A, B) (MAX((A), MIN((V), (B))))
#define CLAMP_CEIL(V, A) MIN(V, A)
#define CLAMP_FLOOR(V, A) MAX(V, A)
#define ABS(V) ((V) < 0 ? -(V) : (V))

#define HANDLE_INVALID MAX_U32

#define STMT(S) do { S; } while(0)

#define STRINGIFY(S) #S     // For macro arg expansion
#define CONCATENATE_IMPL(A, B) A##B
#define CONCATENATE(A, B) CONCATENATE_IMPL(A, B)

//TODO(caio): Verify and test alignment macros further
#define IS_POW2(V) (((V) & ((V) - 1)) == 0)
#define ALIGN_TO(SIZE, BOUND) (((SIZE) + (BOUND) - 1) & ~((BOUND) - 1))   // Aligns to powers of 2 only
#define IS_ALIGNED(SIZE, BOUND) ((u64)(SIZE) % (BOUND) == 0)

#define ARR_LEN(arr) (sizeof(arr)/sizeof(*(arr)))   // # of elements in array
#define ARR_SIZE(arr) (sizeof(arr))                 // Total size in bytes of array

#define OFFSET_IN(type, member) ((u64)&((type*)0)->member)  // Offset of member variable in struct type

#define SET_BIT(x, pos) ((x) |= (1UL << (pos)))
#define CLEAR_BIT(x, pos) ((x) &= (~(1UL << (pos))))
#define TOGGLE_BIT(x, pos) ((x) ^= (1UL << (pos)))
#define CHECK_BIT(x, pos) ((x) & (1UL << (pos)))

#define ENUM_FLAGS(TYPE, FLAGS) ((TYPE)(FLAGS))
#define ENUM_HAS_FLAG(FLAGS, F) ((FLAGS) & (F))

#define DEFAULT_ARRAY(ARR, SIZE) for(i32 _arr = 0; _arr < (SIZE); _arr++) { (ARR)[_arr] = {}; }

};
