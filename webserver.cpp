//
// Created by valdemar on 17.07.16.
//

#include "webserver.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <cerrno>
#include <cstdio>

volatile bool g_web_server_stop = false;
const int MAX_EVENTS = 32;

int set_nonblock(int fd) {
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
        flags = 0;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

int web_server_run(const options &ws_opts) {
    int master_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (master_fd < 0) {
        perror("Error: ");
        return -1;
    }

    sockaddr_in master_addr;
    master_addr.sin_family = AF_INET;
    master_addr.sin_addr = ws_opts.server_ip;
    master_addr.sin_port = ws_opts.server_port;

    if (bind(master_fd, reinterpret_cast<sockaddr *>(&master_addr), sizeof(master_addr)) < 0) {
        perror("Error: ");
        return -2;
    }

    set_nonblock(master_fd);
    listen(master_fd, SOMAXCONN);

    int epl = epoll_create1(0);
    epoll_event ev;
    ev.data.fd = master_fd;
    ev.events = EPOLLIN;
    epoll_ctl(epl, EPOLL_CTL_ADD, master_fd, &ev);

    epoll_event evnts[MAX_EVENTS];

    while (!g_web_server_stop) {
        int evcnt = epoll_wait(epl, evnts, MAX_EVENTS, -1);
        for (int i = 0; (!g_web_server_stop) && i < evcnt; ++i) {
            if (evnts[i].data.fd == master_fd) {
                int sock = accept(master_fd, nullptr, nullptr);
                set_nonblock(sock);
                ev.data.fd = sock;
                ev.events = EPOLLIN;
                epoll_ctl(epl, EPOLL_CTL_ADD, sock, &ev);
                continue;
            }

            if ((evnts[i].events & EPOLLERR) || (evnts[i].events & EPOLLHUP)) {
                epoll_ctl(epl, EPOLL_CTL_DEL, evnts[i].data.fd, nullptr);
                shutdown(evnts[i].data.fd, SHUT_RDWR);
                close(evnts[i].data.fd);
            } else if (evnts[i].events & EPOLLIN) {
                /*
                 * To be multithreaded:
                 * Add to serving socket queue
                 * Notify all threads
                 */
                serve_socket(evnts[i].data.fd);
            }
        }
    }

    printf("Info: Release resources\n");
    return 0;
}


int serve_socket(int sock_fd) {
    char buf[4096];
    ssize_t nbytes = recv(sock_fd, &buf, 4096, 0);
    if ((nbytes <= 0) && (errno != EAGAIN)) {
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
        return -1;
    } else if (nbytes > 0) {
        //Here parse http and create response
        //printf("Info: Message '%.100s...'\n", buf);

        //Echo server
        send(sock_fd, buf, static_cast<size_t>(nbytes), 0);
    }
    return 0;
}