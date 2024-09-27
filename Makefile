all: main

main: main.c acceptor.c acceptor.h log.c log.h event_loop.h event_loop.c \
	event_dispatcher.h epoll_dispatcher.c channel_map.c channel_map.h 	 \
	channel.c channel.h

clean:
	rm -f main