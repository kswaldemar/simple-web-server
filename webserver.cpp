//
// Created by valdemar on 17.07.16.
//

#include "webserver.h"
#include "httpparser.h"

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <queue>
#include <mutex>

volatile bool g_web_server_stop = false;
const char *g_webserver_root_path;

const int MAX_EVENTS = 32;

struct {
    std::queue<int> que;
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
} g_handle_socks;

pthread_cond_t g_condvar;
pthread_mutex_t g_condvar_mtx = PTHREAD_MUTEX_INITIALIZER;

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

void *worker_main(void *) {
    int process_fd;
    pthread_mutex_lock(&g_condvar_mtx);
    while (!g_web_server_stop) {
        pthread_cond_wait(&g_condvar, &g_condvar_mtx);

        if (g_web_server_stop) {
            break;
        }

        pthread_mutex_lock(&g_handle_socks.mtx);
        if (!g_handle_socks.que.empty()) {
            process_fd = g_handle_socks.que.front();
            g_handle_socks.que.pop();
        } else {
            process_fd = 0;
        }
        pthread_mutex_unlock(&g_handle_socks.mtx);

        if (g_web_server_stop) {
            break;
        }

        if (process_fd != 0) {
            pthread_mutex_unlock(&g_condvar_mtx);
            serve_socket(process_fd, g_webserver_root_path);
            pthread_mutex_lock(&g_condvar_mtx);
        }

        if (g_web_server_stop) {
            break;
        }
    }
    pthread_mutex_unlock(&g_condvar_mtx);
    return nullptr;
}

int web_server_run(const options &ws_opts) {

    sockaddr_in master_addr;
    master_addr.sin_family = AF_INET;
    master_addr.sin_addr = ws_opts.server_ip;
    master_addr.sin_port = ws_opts.server_port;
    g_webserver_root_path = ws_opts.server_root;

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

    if (pthread_cond_init(&g_condvar, nullptr) != 0) {
        perror("pthread_cond_init: ");
    }

    std::vector<pthread_t> workers;
    workers.resize(ws_opts.workers_count);
    for (int i = 0; i < ws_opts.workers_count; ++i) {
        pthread_create(&workers[i], nullptr, worker_main, nullptr);
    }

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
                pthread_mutex_lock(&g_condvar_mtx);

                pthread_mutex_lock(&g_handle_socks.mtx);
                g_handle_socks.que.push(evnts[i].data.fd);
                epoll_ctl(epl, EPOLL_CTL_DEL, evnts[i].data.fd, nullptr);
                pthread_mutex_unlock(&g_handle_socks.mtx);

                pthread_cond_signal(&g_condvar);

                pthread_mutex_unlock(&g_condvar_mtx);
            }
        }
    }

    printf("Info: Release resources\n");
    for (const auto &wrk : workers) {
        pthread_join(wrk, nullptr);
    }
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

        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
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
                fclose(msg.req_file);
                return -2;
            }
            sz = fread(buf, 1, sizeof(buf), msg.req_file);
        }
        fclose(msg.req_file);
    }
    return 0;
}
