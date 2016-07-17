//
// Created by valdemar on 17.07.16.
//

#include "webserver.h"

#include <thread>
#include <chrono>

volatile bool g_web_server_stop = false;

int web_server_run(const options &ws_opts) {
    while (!g_web_server_stop) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    printf("Info: Release resources\n");
    return 0;
}
