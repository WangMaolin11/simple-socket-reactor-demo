#include <sys/epoll.h>
#include <time.h>
#include "event_dispatcher.h"
#include "event_loop.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "log.h"

#define MAXEVENTS 128

typedef struct
{
    int event_count;
    int nfds;
    int realloc_copy;
    int efd;
    struct epoll_event *events;
} epoll_dispatcher_data;

static void *epoll_init(struct event_loop *);

static int epoll_add(struct event_loop *, struct channel *channel1);

static int epoll_del(struct event_loop *, struct channel *channel1);

static int epoll_update(struct event_loop *, struct channel *channel1);

static int epoll_dispatch(struct event_loop *, struct timeval *);

static void epoll_clear(struct event_loop *);

const struct event_dispatcher epoll_dispatcher = {
    "epoll",
    epoll_init,
    epoll_add,
    epoll_del,
    epoll_update,
    epoll_dispatch,
    epoll_clear,
};

void *epoll_init(struct event_loop *eventLoop)
{
    epoll_dispatcher_data *epollDispatcherData = malloc(sizeof(epoll_dispatcher_data));

    epollDispatcherData->event_count = 0;
    epollDispatcherData->nfds = 0;
    epollDispatcherData->realloc_copy = 0;
    epollDispatcherData->efd = 0;

    epollDispatcherData->efd = epoll_create1(0);
    if (epollDispatcherData->efd == -1)
    {
        error(1, errno, "epoll create failed");
    }

    epollDispatcherData->events = calloc(MAXEVENTS, sizeof(struct epoll_event));

    return epollDispatcherData;
}

int epoll_add(struct event_loop *eventLoop, struct channel *channel1)
{
    epoll_dispatcher_data *pollDispatcherData = (epoll_dispatcher_data *)eventLoop->event_dispatcher_data;

    int fd = channel1->fd;

    int events = 0;
    if (channel1->events & EVENT_READ)
    {
        events = events | EPOLLIN;
    }
    if (channel1->events & EVENT_WRITE)
    {
        events = events | EPOLLOUT;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;

    if (epoll_ctl(pollDispatcherData->efd, EPOLL_CTL_ADD, fd, &event) == -1)
    {
        error(1, errno, "epoll_ctl add  fd failed");
    }

    return 0;
}

int epoll_del(struct event_loop *eventLoop, struct channel *channel1)
{
    epoll_dispatcher_data *pollDispatcherData = (epoll_dispatcher_data *)eventLoop->event_dispatcher_data;

    int fd = channel1->fd;

    int events = 0;
    if (channel1->events & EVENT_READ)
    {
        events = events | EPOLLIN;
    }
    if (channel1->events & EVENT_WRITE)
    {
        events = events | EPOLLOUT;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;

    if (epoll_ctl(pollDispatcherData->efd, EPOLL_CTL_DEL, fd, &event) == -1)
    {
        error(1, errno, "epoll_ctl add  fd failed");
    }

    return 0;
}

int epoll_update(struct event_loop *eventLoop, struct channel *channel1)
{
    epoll_dispatcher_data *pollDispatcherData = (epoll_dispatcher_data *)eventLoop->event_dispatcher_data;

    int fd = channel1->fd;

    int events = 0;
    if (channel1->events & EVENT_READ)
    {
        events = events | EPOLLIN;
    }
    if (channel1->events & EVENT_WRITE)
    {
        events = events | EPOLLOUT;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;

    if (epoll_ctl(pollDispatcherData->efd, EPOLL_CTL_MOD, fd, &event) == -1)
    {
        error(1, errno, "epoll_ctl add  fd failed");
    }

    return 0;
}

int epoll_dispatch(struct event_loop *eventLoop, struct timeval *timeval1)
{
    epoll_dispatcher_data *epollDispatcherData = (epoll_dispatcher_data *)eventLoop->event_dispatcher_data;

    int i, n;
    n = epoll_wait(epollDispatcherData->efd, epollDispatcherData->events, MAXEVENTS, -1);
    for (i = 0; i < n; i++)
    {
        if ((epollDispatcherData->events[i].events & EPOLLERR) || (epollDispatcherData->events[i].events & EPOLLHUP))
        {
            fprintf(stderr, "epoll error\n");
            close(epollDispatcherData->events[i].data.fd);
            continue;
        }

        if (epollDispatcherData->events[i].events & EPOLLIN)
        {
            printf("fd readable %d\n", epollDispatcherData->events[i].data.fd);
            // channel_event_activate(eventLoop, epollDispatcherData->events[i].data.fd, EVENT_READ);
        }

        if (epollDispatcherData->events[i].events & EPOLLOUT)
        {
             printf("fd writable %d\n", epollDispatcherData->events[i].data.fd);
            // channel_event_activate(eventLoop, epollDispatcherData->events[i].data.fd, EVENT_WRITE);
        }
    }

    return 0;
}

void epoll_clear(struct event_loop *eventLoop)
{
    epoll_dispatcher_data *epollDispatcherData = (epoll_dispatcher_data *)eventLoop->event_dispatcher_data;

    free(epollDispatcherData->events);
    close(epollDispatcherData->efd);
    free(epollDispatcherData);
    eventLoop->event_dispatcher_data = NULL;

    return;
}