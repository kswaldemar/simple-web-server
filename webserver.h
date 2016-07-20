//
// Created by valdemar on 17.07.16.
//

#ifndef SIMPLE_WEB_SERVER_WEBSERVER_H
#define SIMPLE_WEB_SERVER_WEBSERVER_H

#include "optparse.h"

#include <cstdint>

struct http_response_msg;

//True if we should stop right now
extern volatile bool g_web_server_stop;

int web_server_run(const options &ws_opts);

int serve_socket(int sock_fd, const char *webserver_root);

int write_response(int socket_fd, const http_response_msg &msg);

#endif //SIMPLE_WEB_SERVER_WEBSERVER_H
