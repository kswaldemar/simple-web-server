//
// Created by valdemar on 17.07.16.
//

#ifndef SIMPLE_WEB_SERVER_WEBSERVER_H
#define SIMPLE_WEB_SERVER_WEBSERVER_H

#include "optparse.h"

#include <cstdint>

//True if we should stop right now
extern volatile bool g_web_server_stop;

int web_server_run(const options &ws_opts);

int serve_socket(int sock_fd);

#endif //SIMPLE_WEB_SERVER_WEBSERVER_H
