//
// Created by valdemar on 17.07.16.
//

#include "webserver.h"

#include <thread>

#include <fcntl.h>
#include <unistd.h>
#include <set>
#include <algorithm>

volatile bool g_web_server_stop = false;

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

    std::set<int> conn_socks;

    set_nonblock(master_fd);

    listen(master_fd, SOMAXCONN);

    while (!g_web_server_stop) {
        fd_set sock_set;
        FD_ZERO(&sock_set);
        FD_SET(master_fd, &sock_set);

        for (auto &sck : conn_socks) {
            FD_SET(sck, &sock_set);
        }

        int max_fd = std::max(master_fd, *std::max_element(conn_socks.begin(), conn_socks.end()));
        select(max_fd + 1, &sock_set, nullptr, nullptr, nullptr);

        if (g_web_server_stop) {
            break;
        }

        for (auto &sck : conn_socks) {
            if (FD_ISSET(sck, &sock_set)) {
                //Information to one of served socket passed
                if (serve_socket(sck) < 0) {
                    conn_socks.erase(sck);
                }

                /*
                 * To be multithreaded:
                 * Add to serving socket queue
                 * Notify all threads
                 */
            }
        }
        if (FD_ISSET(master_fd, &sock_set)) {
            int sock = accept(master_fd, nullptr, nullptr);
            set_nonblock(sock);
            conn_socks.insert(sock);
        }

    }

    printf("Info: Release resources\n");
    for (auto &sck : conn_socks) {
        shutdown(sck, SHUT_RDWR);
        close(sck);
    }
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
        printf("Info: Message '%.100s...'\n", buf);
    }
    return 0;
}