// ========================================================
// ASYNC
// Threads, job system and asynchronous programming utilities.
// Still needs much improvement.
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "engine/core/base.hpp"

namespace ty
{
namespace async
{

// ========================================================
// [MUTEX]

#define MUTEX_INVALID MAX_U64

struct Lock
{
    u64 uid = MUTEX_INVALID;

    void Acquire();
    void Release();
};

Lock CreateLock();

//TODO(caio): Implement more synchronization primitives (semaphore, condition variable...)

// ========================================================
// [JOB SYSTEM]
// User can define tasks, which are functions declared with a specific signature, and have a fixed number of
// void* params which the user can use via macros.
// Tasks are added to the async queue and grabbed by a worker thread as soon as possible.
// User can either continue doing work on main thread, or wait for all tasks to finish when needed.

#define ASYNC_CALLBACK(CallbackName) void CallbackName(void* _callbackData0, void* _callbackData1, void* _callbackData2, void* _callbackData3, void* _callbackData4)
typedef ASYNC_CALLBACK(Callback);  // AsyncCallback is a function pointer to a function with the above signature.
#define ASYNC_CALLBACK_ARG(VarType, VarName, N) VarType VarName = *(VarType*)(&_callbackData##N)  // Used to convert to/from void* for AsyncCallbacks arguments.

struct Task
{
    Callback* callback     = NULL;
    void* _callbackData0        = NULL;
    void* _callbackData1        = NULL;
    void* _callbackData2        = NULL;
    void* _callbackData3        = NULL;
    void* _callbackData4        = NULL;
};

#define ASYNC_MAX_TASKS 256
#define ASYNC_WORKER_THREAD_COUNT 8     //TODO(caio): Have worker thread count be based on CPU specs.
//#define MEMORY_BARRIER()    MemoryBarrier()

struct TaskQueue
{
    volatile u32 nextTaskToRead     = 0;
    volatile u32 nextTaskToWrite    = 0;
    volatile u32 taskAddedCount     = 0;
    volatile u32 taskCompletedCount = 0;

    // Platform-specific
    // TODO(caio): Remove this whenever these are implemented as types
    HANDLE              semaphore;
    CRITICAL_SECTION    waitAllSection;
    CONDITION_VARIABLE  waitAllCond;

    Task tasks[ASYNC_MAX_TASKS];
};

struct WorkerThreadData
{
    u32 threadID = 0;
    TaskQueue* taskQueue;
};

inline TaskQueue           taskQueue;
inline WorkerThreadData    workerThreadData[ASYNC_WORKER_THREAD_COUNT];

void InitJobSystem();
void AddToWorkerQueue(Task task);
void WaitForAllTasks();

// Platform-specific
DWORD WINAPI WorkerThreadProc(void* data);

};
};
