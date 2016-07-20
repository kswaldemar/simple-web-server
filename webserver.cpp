//
// Created by valdemar on 17.07.16.
//

#include "webserver.h"
#include "httpparser.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <cstring>

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

int create_master_socket(const sockaddr_in &master_addr) {
    int master_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (master_fd < 0) {
        perror("Error: ");
        return -1;
    }

    if (bind(master_fd, reinterpret_cast<const sockaddr *>(&master_addr), sizeof(master_addr)) < 0) {
        perror("Error: ");
        return -2;
    }

    set_nonblock(master_fd);
    listen(master_fd, SOMAXCONN);
    return master_fd;
}

int web_server_run(const options &ws_opts) {

    sockaddr_in master_addr;
    master_addr.sin_family = AF_INET;
    master_addr.sin_addr = ws_opts.server_ip;
    master_addr.sin_port = ws_opts.server_port;

    int master_fd = create_master_socket(master_addr);
    if (master_fd < 0) {
        return -1;
    }

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
                serve_socket(evnts[i].data.fd, ws_opts.server_root);
            }
        }
    }

    printf("Info: Release resources\n");
    return 0;
}


int serve_socket(int sock_fd, const char *webserver_root) {
    char buf[4096];
    ssize_t nbytes = recv(sock_fd, &buf, 4096, 0);
    if ((nbytes <= 0) && (errno != EAGAIN)) {
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
        return -1;
    } else if (nbytes > 0) {
        http_response_msg rsp;
        parse_request(buf, rsp, webserver_root);
        write_response(sock_fd, rsp);
    }
    return 0;
}

int write_response(int socket_fd, const http_response_msg &msg) {
    std::string tosend;
    std::string line_ending = "\r\n";

    tosend += msg.status_line;
    tosend += line_ending;
    for (const auto &header : msg.headers) {
        tosend += header;
        tosend += line_ending;
    }

    tosend += line_ending;

    if (send(socket_fd, tosend.c_str(), tosend.size(), 0) < 0) {
        perror("Cannot send to socket in write_response: ");
        return -1;
    }

    if (msg.req_file) {
        rewind(msg.req_file);

        uint8_t buf[1024];
        int sz = fread(buf, 1, sizeof(buf), msg.req_file);
        while (sz > 0) {
            if (send(socket_fd, buf, sz, 0) < 0) {
                perror("Send file failed: ");
                return -2;
            }
            sz = fread(buf, 1, sizeof(buf), msg.req_file);
        }
        fclose(msg.req_file);
    }
    return 0;
}
