#include "thread_queue.h"
#include "platform/platform.h"
#include <stdlib.h>

u32 global_thread_count = 0;

ThreadTask thread_task(void (*task_callback)(void* data), void* data)
{
    ThreadTask task = { .task_callback = task_callback, .data = data };
    return task;
}

void semaphore_counter_wait(SemaphoreCounter* semaphore_counter)
{
    if (semaphore_counter->count)
    {
        ftic_assert(semaphore_counter->semaphore);
        for (u32 i = 0; i < semaphore_counter->count; i++)
        {
            platform_semaphore_wait_and_decrement(semaphore_counter->semaphore);
        }
        semaphore_counter->count = 0;
    }
}

void semaphore_counter_wait_and_free(SemaphoreCounter* semaphore_counter)
{
    semaphore_counter_wait(semaphore_counter);
    platform_semaphore_destroy(semaphore_counter->semaphore);
    free(semaphore_counter->semaphore);
}

void thread_task_push_(ThreadTaskQueue* task_queue, ThreadTask task,
                       FTicSemaphore* semaphore)
{
    platform_semaphore_wait_and_decrement(&task_queue->mutex);

    // TODO: make it growing or something else
    if (task_queue->size < task_queue->capacity)
    {
        task_queue->tail %= task_queue->capacity;
        task_queue->tasks[task_queue->tail] =
            (ThreadTaskInternal){ .task = task, .semaphore = semaphore };

        task_queue->tail++;
        task_queue->size++;

        u64* count = hash_table_get_uu64(&task_queue->id_to_count, task.id);
        if (count)
        {
            ++(*count);
        }
        else
        {
            hash_table_insert_uu64(&task_queue->id_to_count, task.id, 1);
        }
    }

    platform_semaphore_increment(&task_queue->mutex, NULL);

    platform_semaphore_increment(&task_queue->start_semaphore, NULL);
}

void thread_tasks_push(ThreadTaskQueue* task_queue, ThreadTask* tasks,
                       u32 task_count, SemaphoreCounter* semaphore_counter)
{
    if (semaphore_counter)
    {
        if (!semaphore_counter->semaphore && task_count)
        {
            semaphore_counter->semaphore =
                (FTicSemaphore*)calloc(1, sizeof(FTicSemaphore));
            *semaphore_counter->semaphore =
                platform_semaphore_create(0, task_count);
        }
        semaphore_counter->count = task_count;
    }
    for (u32 i = 0; i < task_count; i++)
    {
        thread_task_push_(task_queue, tasks[i],
                          semaphore_counter ? semaphore_counter->semaphore
                                            : NULL);
    }
}

internal ThreadTaskInternal thread_task_pop(ThreadTaskQueue* task_queue)
{
    platform_semaphore_wait_and_decrement(&task_queue->mutex);
    ThreadTaskInternal task = { 0 };

    ftic_assert(task_queue->size > 0);

    task_queue->head %= task_queue->capacity;
    task = task_queue->tasks[task_queue->head];

    task_queue->head++;
    task_queue->size--;
    u64* count = hash_table_get_uu64(&task_queue->id_to_count, task.task.id);
    if (count)
    {
        --(*count);
    }

    platform_semaphore_increment(&task_queue->mutex, NULL);
    return task;
}

thread_return_value thread_loop(void* data)
{
    ThreadAttrib* attrib = (ThreadAttrib*)data;

    for (;;)
    {
        if (platform_interlock_compare_exchange(&attrib->stop_flag, 0, 0))
        {
            break;
        }

        platform_semaphore_wait_and_decrement(attrib->start_semaphore);

        if (platform_interlock_compare_exchange(&attrib->stop_flag, 0, 0))
        {
            break;
        }

        ThreadTaskInternal task = thread_task_pop(attrib->queue);
        if (task.task.task_callback)
        {
            task.task.task_callback(task.task.data);
        }

        if (task.semaphore)
        {
            platform_semaphore_increment(task.semaphore, NULL);
        }
    }
    return 0;
}

u64 thread_get_task_count(ThreadTaskQueue* task_queue, u64 id)
{
    platform_semaphore_wait_and_decrement(&task_queue->mutex);

    u64* count = hash_table_get_uu64(&task_queue->id_to_count, id);

    platform_semaphore_increment(&task_queue->mutex, NULL);

    return count ? *count : 0;
}

internal u64 hash_function_u64(const void* key, u32 len, u64 seed)
{
    return *((u64*)key);
}

void thread_init(u32 capacity, u32 thread_count, ThreadQueue* queue)
{
    ftic_assert(!queue->pool);
    if (thread_count > 8)
    {
        thread_count = 8;
    }
    queue->pool =
        (FTicThreadHandle*)calloc(thread_count, sizeof(FTicThreadHandle));
    queue->pool_size = thread_count;
    queue->attribs = (ThreadAttrib*)calloc(thread_count, sizeof(ThreadAttrib));
    global_thread_count = thread_count;

    FTicSemaphore start_semaphore = platform_semaphore_create(0, capacity);
    FTicSemaphore mutex = platform_semaphore_create(1, capacity);
    queue->task_queue.start_semaphore = start_semaphore;
    queue->task_queue.mutex = mutex;
    queue->task_queue.capacity = capacity;
    queue->task_queue.tasks = (ThreadTaskInternal*)calloc(
        queue->task_queue.capacity, sizeof(ThreadTaskInternal));
    queue->task_queue.id_to_count =
        hash_table_create_uu64(100, hash_function_u64);

    for (u32 i = 0; i < thread_count; i++)
    {
        ThreadAttrib* ta = queue->attribs + i;
        ta->start_semaphore = &queue->task_queue.start_semaphore;
        ta->queue = &queue->task_queue;
        ta->stop_flag = 0;
        ta->id = i;
        queue->pool[i] = platform_thread_create(ta, thread_loop, 0, NULL);
    }
}

void threads_destroy(ThreadQueue* queue)
{
    const u32 thread_count = queue->pool_size;
    for (u32 i = 0; i < thread_count; i++)
    {
        platform_interlock_exchange(&queue->attribs[i].stop_flag, 1);
    }
    for (u32 i = 0; i < thread_count; i++)
    {
        platform_semaphore_increment(&queue->task_queue.start_semaphore, NULL);
    }
    for (u32 i = 0; i < thread_count; i++)
    {
        platform_thread_join(queue->pool[i]);
        platform_thread_close(queue->pool[i]);
    }
}
