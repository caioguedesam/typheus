#include "engine/core/async.hpp"
#include "engine/core/debug.hpp"

namespace ty
{
namespace async
{

void Lock::Acquire()
{
    i32 result = WaitForSingleObjectEx((HANDLE)uid, INFINITE, FALSE);
    ASSERT(result == WAIT_OBJECT_0);
}

void Lock::Release()
{
    i32 result = ReleaseMutex((HANDLE)uid);
    ASSERT(result);
}

Lock CreateLock()
{
    Lock result = {};
    result.uid = (u64)CreateMutex(NULL, FALSE, NULL);
    ASSERT(result.uid);
    return result;
};

void InitJobSystem()
{
    taskQueue = {};

    // Initializing necessary synchronization primitives
    taskQueue.semaphore = CreateSemaphoreExA(
            0, 0, ASYNC_WORKER_THREAD_COUNT,
            0, 0, SEMAPHORE_ALL_ACCESS);
    ASSERT(taskQueue.semaphore);
    InitializeCriticalSection(&taskQueue.waitAllSection);
    InitializeConditionVariable(&taskQueue.waitAllCond);

    // Initializing worker threads
    for(i32 i = 0; i < ASYNC_WORKER_THREAD_COUNT; i++)
    {
        workerThreadData[i] = {};
        workerThreadData[i].threadID = (u32)i;
        workerThreadData[i].taskQueue = &taskQueue;
        HANDLE threadID = CreateThread(NULL, 0, WorkerThreadProc, &workerThreadData[i], 0, NULL);
        ASSERT(threadID);
    }
}

DWORD WINAPI WorkerThreadProc(void* data)
{
    WorkerThreadData* threadData = (WorkerThreadData*)data;
    TaskQueue* taskQueue = threadData->taskQueue;

    while(true)
    {
        bool hasProcessedTask = false;
        u32 taskToRead = taskQueue->nextTaskToRead;
        u32 nextTaskToRead = (taskToRead + 1) % ASYNC_MAX_TASKS;

        if(taskToRead != taskQueue->nextTaskToWrite)
        {
            u32 taskIndex = InterlockedCompareExchange(
                    (LONG volatile*)&taskQueue->nextTaskToRead,
                    nextTaskToRead,
                    taskToRead);

            if(taskIndex == taskToRead)
            {
                hasProcessedTask = true;
                Task task = taskQueue->tasks[taskIndex];
                // Perform work
                //PROFILE_START_SCOPE("Async Work")
                task.callback(task._callbackData0, task._callbackData1, task._callbackData2,
                                task._callbackData3, task._callbackData4);
                //PROFILE_END_SCOPE

                // Mark work as finished
                InterlockedIncrement((LONG volatile*)&taskQueue->taskCompletedCount);

                WakeAllConditionVariable(&taskQueue->waitAllCond);
            }
        }

        if(!hasProcessedTask)
        {
            // No available tasks in the task queue, go to sleep until new task is added.
            WaitForSingleObjectEx(taskQueue->semaphore, INFINITE, FALSE);
        }
    }
}

void AddToWorkerQueue(Task task)
{
    u32 taskToWrite = taskQueue.nextTaskToWrite;
    u32 nextTaskToWrite = (taskToWrite + 1) % ASYNC_MAX_TASKS;
    ASSERT(nextTaskToWrite != taskQueue.nextTaskToRead);

    taskQueue.tasks[taskToWrite] = task;
    taskQueue.taskAddedCount++;

    // Ensure next write index is not incremented before task data is written to queue
    //MEMORY_BARRIER();
    MemoryBarrier();

    taskQueue.nextTaskToWrite = nextTaskToWrite;

    // Signal worker threads that there's available work
    ReleaseSemaphore(taskQueue.semaphore, 1, 0);
}

void WaitForAllTasks()
{
    EnterCriticalSection(&taskQueue.waitAllSection);

    while(taskQueue.taskCompletedCount != taskQueue.taskAddedCount)
    {
        SleepConditionVariableCS(&taskQueue.waitAllCond, &taskQueue.waitAllSection, INFINITE);
    }

    LeaveCriticalSection(&taskQueue.waitAllSection);
}

};
};
