#include "event_loop.h"
#include "log.h"
#include "channel_map.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

int handleWakeup(void *data)
{
    struct event_loop *eventLoop = (struct event_loop *)data;
    char one;
    ssize_t n = read(eventLoop->socketPair[1], &one, sizeof one);
    if (n != sizeof one)
    {
        error(1, errno, "xx");
    }
}

struct event_loop *event_loop_init()
{
    return event_loop_init_with_name(NULL);
}

struct event_loop *event_loop_init_with_name(char *thread_name)
{
    struct event_loop *eventLoop = malloc(sizeof(struct event_loop));

    eventLoop->quit = 0;
    eventLoop->eventDispatcher = &epoll_dispatcher;
    eventLoop->event_dispatcher_data = eventLoop->eventDispatcher->init(eventLoop);

    eventLoop->channelMap = malloc(sizeof(struct channel_map));
    map_init(eventLoop->channelMap);

    eventLoop->is_handle_pending = 0;
    eventLoop->pending_head = NULL;
    eventLoop->pending_tail = NULL;

    eventLoop->owner_thread_id = pthread_self();
    pthread_mutex_init(&eventLoop->mutex, NULL);
    pthread_cond_init(&eventLoop->cond, NULL);

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, eventLoop->socketPair) < 0)
    {
        error(1, errno, "socketpair set fialed");
    }

    if (thread_name == NULL)
    {
        eventLoop->thread_name = "main thread";
    }
    else
    {
        eventLoop->thread_name = thread_name;
    }

    // 加入到 channel 里并设置回调
    struct channel *channel = channel_new(eventLoop->socketPair[1], EVENT_READ, handleWakeup, NULL, eventLoop);
    event_loop_add_channel_event(eventLoop, eventLoop->socketPair[1], channel);

    return eventLoop;
}

void event_loop_wakeup(struct event_loop *eventLoop)
{
    char one = 'a';
    ssize_t n = write(eventLoop->socketPair[0], &one, sizeof one);
    if (n != sizeof one)
    {
        error(1, errno, "wakeup event loop thread failed");
    }
}

void event_loop_channel_buffer_nolock(struct event_loop *eventLoop, int fd, struct channel *channel1, int type)
{
    // add channel into the pending list
    struct channel_element *channelElement = malloc(sizeof(struct channel_element));
    channelElement->channel = channel1;
    channelElement->type = type;
    channelElement->next = NULL;
    // 第一个元素
    if (eventLoop->pending_head == NULL)
    {
        eventLoop->pending_head = eventLoop->pending_tail = channelElement;
    }
    else
    {
        eventLoop->pending_tail->next = channelElement;
        eventLoop->pending_tail = channelElement;
    }
}

int event_loop_handle_pending_channel(struct event_loop *eventLoop)
{
    pthread_mutex_lock(&eventLoop->mutex);
    eventLoop->is_handle_pending = 1;

    struct channel_element *channelElement = eventLoop->pending_head;
    while (channelElement != NULL)
    {
        // save into event_map
        struct channel *channel = channelElement->channel;
        int fd = channel->fd;
        if (channelElement->type == 1)
        {
            event_loop_handle_pending_add(eventLoop, fd, channel);
        }
        else if (channelElement->type == 2)
        {
            event_loop_handle_pending_remove(eventLoop, fd, channel);
        }
        else if (channelElement->type == 3)
        {
            event_loop_handle_pending_update(eventLoop, fd, channel);
        }
        channelElement = channelElement->next;
    }

    eventLoop->pending_head = eventLoop->pending_tail = NULL;
    eventLoop->is_handle_pending = 0;

    // release the lock
    pthread_mutex_unlock(&eventLoop->mutex);

    return 0;
}

int event_loop_do_channel_event(struct event_loop *eventLoop, int fd, struct channel *channel1, int type)
{
    // get the lock
    pthread_mutex_lock(&eventLoop->mutex);
    assert(eventLoop->is_handle_pending == 0);
    event_loop_channel_buffer_nolock(eventLoop, fd, channel1, type);
    // release the lock
    pthread_mutex_unlock(&eventLoop->mutex);
    if (eventLoop->owner_thread_id != pthread_self())
    {
        event_loop_wakeup(eventLoop);
    }
    else
    {
        event_loop_handle_pending_channel(eventLoop);
    }

    return 0;
}

int event_loop_add_channel_event(struct event_loop *eventLoop, int fd, struct channel *channel1)
{
    return event_loop_do_channel_event(eventLoop, fd, channel1, 1);
}

int event_loop_remove_channel_event(struct event_loop *eventLoop, int fd, struct channel *channel1)
{
    return event_loop_do_channel_event(eventLoop, fd, channel1, 2);
}

int event_loop_update_channel_event(struct event_loop *eventLoop, int fd, struct channel *channel1)
{
    return event_loop_do_channel_event(eventLoop, fd, channel1, 3);
}

int event_loop_handle_pending_add(struct event_loop *eventLoop, int fd, struct channel *channel)
{
    struct channel_map *map = eventLoop->channelMap;

    if (fd < 0)
        return 0;

    if (fd >= map->nentries)
    {
        if (map_make_space(map, fd, sizeof(struct channel *)) == -1)
            return (-1);
    }

    // 第一次创建，增加
    if ((map)->entries[fd] == NULL)
    {
        map->entries[fd] = channel;
        // add channel
        struct event_dispatcher *eventDispatcher = eventLoop->eventDispatcher;
        eventDispatcher->add(eventLoop, channel);
        return 1;
    }

    return 0;
}

int event_loop_handle_pending_remove(struct event_loop *eventLoop, int fd, struct channel *channel)
{
    struct channel_map *map = eventLoop->channelMap;

    if (fd < 0)
        return 0;

    if (fd >= map->nentries)
        return (-1);

    struct channel *channel2 = map->entries[fd];

    // update dispatcher(multi-thread)here
    struct event_dispatcher *eventDispatcher = eventLoop->eventDispatcher;

    int retval = 0;
    if (eventDispatcher->del(eventLoop, channel2) == -1)
    {
        retval = -1;
    }
    else
    {
        retval = 1;
    }

    map->entries[fd] = NULL;
    return retval;
}

int event_loop_handle_pending_update(struct event_loop *eventLoop, int fd, struct channel *channel)
{
    struct channel_map *map = eventLoop->channelMap;

    if (fd < 0)
        return 0;

    if ((map)->entries[fd] == NULL)
    {
        return (-1);
    }

    // update channel
    struct event_dispatcher *eventDispatcher = eventLoop->eventDispatcher;
    eventDispatcher->update(eventLoop, channel);
    return 1;
}

int channel_event_activate(struct event_loop *eventLoop, int fd, int revents)
{
    struct channel_map *map = eventLoop->channelMap;

    if (fd < 0)
        return 0;

    if (fd >= map->nentries)
        return (-1);

    struct channel *channel = map->entries[fd];

    if (revents & (EVENT_READ))
    {
        if (channel->eventReadCallback)
            channel->eventReadCallback(channel->data);
    }
    if (revents & (EVENT_WRITE))
    {
        if (channel->eventWriteCallback)
            channel->eventWriteCallback(channel->data);
    }

    return 0;
}

int event_loop_run(struct event_loop *eventLoop)
{
    struct event_dispatcher *dispatcher = eventLoop->eventDispatcher;

    if (eventLoop->owner_thread_id != pthread_self())
    {
        exit(1);
    }

    struct timeval timeval1;
    timeval1.tv_sec = 1;

    while (!eventLoop->quit)
    {
        // block here to wait I/O event, and get active channels
        dispatcher->dispatch(eventLoop, &timeval1);

        // handle the pending channel
        event_loop_handle_pending_channel(eventLoop);
    }

    return 0;
}
