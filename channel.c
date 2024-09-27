#include "channel.h"
#include <stdlib.h>

struct channel *
channel_new(int fd, int events, event_read_callback eventReadCallback, event_write_callback eventWriteCallback,
            void *data)
{
    struct channel *chan = malloc(sizeof(struct channel));
    chan->fd = fd;
    chan->events = events;
    chan->eventReadCallback = eventReadCallback;
    chan->eventWriteCallback = eventWriteCallback;
    chan->data = data;
    return chan;
}