#include "./time.hpp"
#include "./debug.hpp"

namespace ty
{
namespace time
{

Context MakeTimeContext()
{
    LARGE_INTEGER frequency;
    BOOL ret = QueryPerformanceFrequency(&frequency);
    ASSERT(ret);

    Context result = {};
    result.ticksPerSecond = frequency.QuadPart;
    return result;
}

Timer MakeTimer(Context* ctx)
{
    Timer result = {};
    result.frequency = ctx->ticksPerSecond;
    return result;
}

void StartTimer(Timer* timer)
{
    ASSERT(timer);
    LARGE_INTEGER counter;
    BOOL ret = QueryPerformanceCounter(&counter);
    ASSERT(ret);
    timer->startTick = counter.QuadPart;
}

void EndTimer(Timer* timer)
{
    ASSERT(timer && timer->startTick != TIMER_INVALID);
    LARGE_INTEGER counter;
    BOOL ret = QueryPerformanceCounter(&counter);
    ASSERT(ret);
    timer->endTick = counter.QuadPart;
}

u64 GetElapsedTicks(Timer* timer)
{
    ASSERT(timer->startTick != TIMER_INVALID);
    ASSERT(timer->endTick != TIMER_INVALID);
    return timer->endTick - timer->startTick;
}

f64 GetElapsedSec(Timer* timer)
{
    ASSERT(timer && timer->frequency != TIMER_INVALID);
    return (f64)(GetElapsedTicks(timer)) / (f64)(timer->frequency);
}

f64 GetElapsedMSec(Timer* timer)
{
    ASSERT(timer && timer->frequency != TIMER_INVALID);
    return (f64)(GetElapsedTicks(timer) * (u64)1e3) / (f64)(timer->frequency);
}

f64 GetElapsedNSec(Timer* timer)
{
    ASSERT(timer && timer->frequency != TIMER_INVALID);
    return (f64)(GetElapsedTicks(timer) * (u64)1e9) / (f64)(timer->frequency);
}

};
};
