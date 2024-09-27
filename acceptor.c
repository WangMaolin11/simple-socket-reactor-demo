#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include "acceptor.h"
#include "log.h"

struct acceptor *acceptor_init(int port)
{
    struct acceptor *acceptor1 = malloc(sizeof(struct acceptor));
    acceptor1->listen_port = port;
    acceptor1->listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    fcntl(acceptor1->listen_fd, F_SETFL, O_NONBLOCK);

    if (acceptor1->listen_fd == -1)
        error(1, errno, "socket() failed");

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int on = 1;
    setsockopt(acceptor1->listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    int rt1 = bind(acceptor1->listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (rt1 < 0)
    {
        error(1, errno, "bind failed ");
    }

    int rt2 = listen(acceptor1->listen_fd, 1024);
    if (rt2 < 0)
    {
        error(1, errno, "listen failed ");
    }

    return acceptor1;
}