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

struct Timer
{
    u64 startTick     = TIMER_INVALID;
    u64 endTick       = TIMER_INVALID;

    void Start();
    void Stop();

    u64 GetElapsedTicks();
    f64 GetElapsedS();
    f64 GetElapsedMS();
    f64 GetElapsedNS();
};

void Init();

};
};
