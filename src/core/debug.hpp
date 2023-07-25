// ========================================================
// DEBUG
// Error handling, asserts, logging and any other debug utilities.
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "./base.hpp"

namespace ty
{

// ========================================================
// [ASSERT]

#if _NOASSERT
#define ASSERT(EXPR)
#define ASSERTF(EXPR, ...)
#define STATIC_ASSERT(EXPR)
#else

void Assert(u64 expr, const char* msg);
void AssertFormat(u64 expr, const char* fmt, ...);

#define ASSERT(EXPR) STMT(ty::Assert((ty::u64)(EXPR), STRINGIFY(EXPR)))
#define ASSERTF(EXPR, FMT, ...) STMT(ty::AssertFormat((ty::i32)(EXPR), FMT, __VA_ARGS__))
#define STATIC_ASSERT(EXPR) static_assert((EXPR))

#endif

// ========================================================
// [LOGGING]

#if _NOLOGGING
#define LOG(MSG)
#define LOGL(LABEL, MSG)
#define LOGF(FMT, ...)
#define LOGLF(LABEL, FMT, ...)
#else

void LogFormat(const char* label, const char* fmt, ...);
#define LOG(MSG) STMT(LogFormat("LOG", "%s", MSG))
#define LOGL(LABEL, MSG) STMT(LogFormat(LABEL, "%s", MSG))
#define LOGF(FMT, ...) STMT(LogFormat("LOG", FMT, __VA_ARGS__))
#define LOGLF(LABEL, FMT, ...) STMT(LogFormat(LABEL, FMT, __VA_ARGS__))

#endif
};
