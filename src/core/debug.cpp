#include "./debug.hpp"

namespace ty
{

#ifndef _NOASSERT
void Assert(u64 expr, const char* msg)
{
    if(expr) return;
    MessageBoxExA(
            NULL,
            msg,
            "FAILED ASSERT",
            MB_OK,
            0);
    DebugBreak();
    ExitProcess(-1);
}

void AssertFormat(u64 expr, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[2048];
    vsprintf(buf, fmt, args);
    if(expr) return;
    MessageBoxExA(
            NULL,
            buf,
            "FAILED ASSERT",
            MB_OK,
            0);
    va_end(args);
    DebugBreak();
    ExitProcess(-1);
}

#endif

#ifndef _NOLOGGING
void LogFormat(const char* label, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[2048];
    vsprintf(buf, fmt, args);
    printf("[%s]: %s\n", label, buf);
    va_end(args);
}

#endif

};
