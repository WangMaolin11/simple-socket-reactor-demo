#ifndef _CHANNEL_H
#define _CHANNEL_H

#define EVENT_TIMEOUT 0x01
/** Wait for a socket or FD to become readable */
#define EVENT_READ 0x02
/** Wait for a socket or FD to become writeable */
#define EVENT_WRITE 0x04
/** Wait for a POSIX signal to be raised*/
#define EVENT_SIGNAL 0x08

typedef int (*event_read_callback)(void *data);
typedef int (*event_write_callback)(void *data);

struct channel
{
    int fd;
    int events;
    event_read_callback eventReadCallback;
    event_write_callback eventWriteCallback;
    void *data;
};

struct channel *
channel_new(int fd, int events, event_read_callback eventReadCallback, event_write_callback eventWriteCallback,
            void *data);

#endif