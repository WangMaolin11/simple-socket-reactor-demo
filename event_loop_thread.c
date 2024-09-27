#include "event_loop_thread.h"
#include <pthread.h>
#include <stdlib.h>

void *event_loop_thread_run(void *arg)
{
    struct event_loop_thread *eventLoopThread = (struct event_loop_thread *)arg;

    eventLoopThread->eventLoop = event_loop_init_with_name(eventLoopThread->thread_name);
    pthread_cond_signal(&eventLoopThread->cond);

    pthread_mutex_unlock(&eventLoopThread->mutex);

    // 子线程event loop run
    event_loop_run(eventLoopThread->eventLoop);
}

int event_loop_thread_init(struct event_loop_thread *eventLoopThread, int i)
{
    pthread_mutex_init(&eventLoopThread->mutex, NULL);
    pthread_cond_init(&eventLoopThread->cond, NULL);

    eventLoopThread->eventLoop = NULL;
    eventLoopThread->thread_count = 0;
    eventLoopThread->thread_tid = 0;

    char *buf = malloc(16);
    sprintf(buf, "Thread-%d\0", i + 1);
    eventLoopThread->thread_name = buf;

    return 0;
}

struct event_loop *event_loop_thread_start(struct event_loop_thread *eventLoopThread)
{
    pthread_create(&eventLoopThread->thread_tid, NULL, event_loop_thread_run, eventLoopThread);

    pthread_mutex_lock(&eventLoopThread->mutex);
    while (eventLoopThread->eventLoop == NULL)
        pthread_cond_wait(&eventLoopThread->cond, &eventLoopThread->mutex);

    pthread_mutex_unlock(&eventLoopThread->mutex);

    return eventLoopThread->eventLoop;
}