#pragma once
#include "define.h"

typedef struct ThreadTask
{
    void (*task_callback)(void* data);
    void* data;
} ThreadTask;

typedef struct ThreadTaskInternal
{
    ThreadTask task;
    FTicSemaphore* semaphore;
} ThreadTaskInternal;

typedef struct SemaphoreCounter
{
    FTicSemaphore* semaphore;
    u32 count;
} SemaphoreCounter;

typedef struct ThreadTaskQueue
{
    FTicSemaphore mutex;
    FTicSemaphore start_semaphore;
    ThreadTaskInternal* tasks;
    u32 capacity;
    volatile u32 size;
    volatile u32 head;
    volatile u32 tail;
} ThreadTaskQueue;

typedef struct ThreadAttrib
{
    FTicSemaphore* start_semaphore;
    ThreadTaskQueue* queue;
    volatile long stop_flag;
    u32 id;
} ThreadAttrib;

typedef struct ThreadQueue
{
    u32 pool_size;
    FTicThreadHandle* pool;
    ThreadAttrib* attribs;
    ThreadTaskQueue task_queue;
} ThreadQueue;

#define THREAD_TASK_ENTRY_POINT(function_name) void function_name(void* data)
ThreadTask thread_task(void (*task_callback)(void* data), void* data);
void semaphore_counter_wait(SemaphoreCounter* semaphore_counter);
void semaphore_counter_wait_and_free(SemaphoreCounter* semaphore_counter);
void thread_tasks_push(ThreadTaskQueue* task_queue, ThreadTask* tasks, u32 task_count, SemaphoreCounter* semaphore_counter);
void thread_init(u32 capacity, u32 thread_count, ThreadQueue* queue);
void threads_destroy(ThreadQueue* queue);

extern u32 global_thread_count;
