#include "acceptor.h"
#include "event_loop.h"
#include "event_dispatcher.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{
    struct acceptor *acpt = acceptor_init(8888);

    struct event_loop *eventLoop = event_loop_init();

    struct channel chan;
    chan.events = EVENT_READ;
    chan.fd = acpt->listen_fd;
    chan.data = eventLoop;

    event_loop_add_channel_event(eventLoop, acpt->listen_fd, &chan);

    event_loop_run(eventLoop);

    return 0;
}