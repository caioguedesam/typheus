// ========================================================
// PROFILE
// General profiling hooks and macros.
// @Caio Guedes, 2023
// ========================================================

#pragma once

#ifndef _PROFILE

#define USE_OPTICK 0
#define OPTICK_ENABLE_TRACING 0
#define OPTICK_ENABLE_GPU 0

#define PROFILE_START
#define PROFILE_FRAME
#define PROFILE_SCOPE
#define PROFILE_CAPTURE_START       
#define PROFILE_CAPTURE_STOP        
#define PROFILE_CAPTURE_SAVE(name)  
#define PROFILE_END

#else

#define USE_OPTICK 1
#define OPTICK_ENABLE_TRACING 0
#define OPTICK_ENABLE_GPU 0

#include "../third_party/optick/optick.h"

#define PROFILE_START
#define PROFILE_FRAME               OPTICK_FRAME("MainThread")
#define PROFILE_SCOPE               OPTICK_EVENT() 
#define PROFILE_CAPTURE_START       OPTICK_START_CAPTURE()
#define PROFILE_CAPTURE_STOP        OPTICK_STOP_CAPTURE()
#define PROFILE_CAPTURE_SAVE(name)  OPTICK_SAVE_CAPTURE(name)
#define PROFILE_END                 OPTICK_SHUTDOWN()

#endif
