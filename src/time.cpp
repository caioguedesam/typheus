#include "base.hpp" 
#include "time.hpp"

namespace Sol
{

u64 clockFrequency = TIMER_INVALID;

void InitTime()
{
    LARGE_INTEGER perfFrequency;
    BOOL ret = QueryPerformanceFrequency(&perfFrequency);
    ASSERT(ret);
    clockFrequency = perfFrequency.QuadPart;
}

void StartTimer(Timer* timer)
{
    LARGE_INTEGER perfCounter;
    BOOL ret = QueryPerformanceCounter(&perfCounter);
    ASSERT(ret);
    timer->start = perfCounter.QuadPart;
}

void EndTimer(Timer* timer)
{
    ASSERT(timer->start != TIMER_INVALID);
    LARGE_INTEGER perfCounter;
    BOOL ret = QueryPerformanceCounter(&perfCounter);
    ASSERT(ret);
    timer->end = perfCounter.QuadPart;
}

u64 GetTicks(const Timer& timer)
{
    ASSERT(timer.start != TIMER_INVALID);
    ASSERT(timer.end != TIMER_INVALID);
    return timer.end - timer.start;
}

f64 GetTime_NS(const Timer& timer)
{
    ASSERT(clockFrequency != TIMER_INVALID);
    return (f64)(GetTicks(timer) * (u64)1e9) / (f64)clockFrequency;
}

f64 GetTime_MS(const Timer& timer)
{
    ASSERT(clockFrequency != TIMER_INVALID);
    return (f64)(GetTicks(timer) * (u64)1e3) / (f64)clockFrequency;
}

f64 GetTime_S(const Timer& timer)
{
    ASSERT(clockFrequency != TIMER_INVALID);
    return (f64)GetTicks(timer) / (f64)clockFrequency;
}

} // namespace Sol
