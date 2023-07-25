#include "./time.hpp"
#include "./debug.hpp"

namespace ty
{
namespace time
{

u64 ticksPerSecond = TIMER_INVALID;

void Init()
{
    LARGE_INTEGER frequency;
    BOOL ret = QueryPerformanceFrequency(&frequency);
    ASSERT(ret);
    ticksPerSecond = frequency.QuadPart;
}

void Timer::Start()
{
    LARGE_INTEGER counter;
    BOOL ret = QueryPerformanceCounter(&counter);
    ASSERT(ret);
    startTick = counter.QuadPart;
}

void Timer::Stop()
{
    ASSERT(startTick != TIMER_INVALID);
    LARGE_INTEGER counter;
    BOOL ret = QueryPerformanceCounter(&counter);
    ASSERT(ret);
    endTick = counter.QuadPart;
}

u64 Timer::GetElapsedTicks()
{
    ASSERT(startTick != TIMER_INVALID);
    ASSERT(endTick != TIMER_INVALID);
    return endTick - startTick;
}

f64 Timer::GetElapsedS()
{
    ASSERT(ticksPerSecond != TIMER_INVALID);
    return (f64)(GetElapsedTicks()) / (f64)ticksPerSecond;
}

f64 Timer::GetElapsedMS()
{
    ASSERT(ticksPerSecond != TIMER_INVALID);
    return (f64)(GetElapsedTicks() * (u64)1e3) / (f64)ticksPerSecond;
}

f64 Timer::GetElapsedNS()
{
    ASSERT(ticksPerSecond != TIMER_INVALID);
    return (f64)(GetElapsedTicks() * (u64)1e9) / (f64)ticksPerSecond;
}

};
};
