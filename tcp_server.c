#include "tcp_server.h"
#include "thread_pool.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>

int handle_connection_established(void *data)
{
    struct TCPserver *tcpServer = (struct TCPserver *)data;
    struct acceptor *acceptor = tcpServer->acceptor;
    int listenfd = acceptor->listen_fd;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connected_fd = accept(listenfd, (struct sockaddr *)&client_addr, &client_len);

    // 设置非阻塞
    fcntl(connected_fd, F_SETFL, O_NONBLOCK);

    struct event_loop *eventLoop = thread_pool_get_loop(tcpServer->threadPool);

    // 创建 tcp_connection

    if (tcpServer->data != NULL)
    {
        // tcpConnection->data = tcpServer->data;
    }
    return 0;
}

struct TCPserver *
tcp_server_init(struct event_loop *eventLoop, struct acceptor *acceptor,
                connection_completed_call_back connectionCompletedCallBack,
                message_call_back messageCallBack,
                write_completed_call_back writeCompletedCallBack,
                connection_closed_call_back connectionClosedCallBack,
                int threadNum)
{
    struct TCPserver *tcpServer = malloc(sizeof(struct TCPserver));
    tcpServer->eventLoop = eventLoop;
    tcpServer->acceptor = acceptor;
    tcpServer->connectionCompletedCallBack = connectionCompletedCallBack;
    tcpServer->messageCallBack = messageCallBack;
    tcpServer->writeCompletedCallBack = writeCompletedCallBack;
    tcpServer->connectionClosedCallBack = connectionClosedCallBack;
    tcpServer->threadNum = threadNum;
    tcpServer->threadPool = thread_pool_new(eventLoop, threadNum);
    tcpServer->data = NULL;

    return tcpServer;
}

void tcp_server_start(struct TCPserver *tcpServer)
{
    struct acceptor *acceptor = tcpServer->acceptor;
    struct event_loop *eventLoop = tcpServer->eventLoop;

    // 开启多个线程
    thread_pool_start(tcpServer->threadPool);

    struct channel *channel = channel_new(acceptor->listen_fd, EVENT_READ, handle_connection_established, NULL,
                                          tcpServer);
    event_loop_add_channel_event(eventLoop, channel->fd, channel);
    return;
}