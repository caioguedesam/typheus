#pragma once
#include "base.hpp"

namespace Sol
{

#define TIMER_INVALID MAX_U64

struct Timer
{
    u64 start   = TIMER_INVALID;
    u64 end     = TIMER_INVALID;
};

void InitTime();

void StartTimer(Timer* timer);
void EndTimer(Timer* timer);

u64 GetTicks(const Timer& timer);
f64 GetTime_NS(const Timer& timer);
f64 GetTime_MS(const Timer& timer);
f64 GetTime_S(const Timer& timer);

}   // namespace Sol
