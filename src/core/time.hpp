// ========================================================
// TIME
// Time measurement utilites.
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "./base.hpp"

namespace ty
{
namespace time
{

// ========================================================
// [TIMER]

#define TIMER_INVALID MAX_U64

struct Context
{
    u64 ticksPerSecond = TIMER_INVALID;
};

Context MakeTimeContext();

struct Timer
{
    u64 startTick     = TIMER_INVALID;
    u64 endTick       = TIMER_INVALID;
    u64 frequency     = TIMER_INVALID;
};

Timer MakeTimer(Context* ctx);
void StartTimer(Timer* timer);
void EndTimer(Timer* timer);

u64 GetElapsedTicks(Timer* timer);
f64 GetElapsedSec(Timer* timer);
f64 GetElapsedMSec(Timer* timer);
f64 GetElapsedNSec(Timer* timer);

};
};
