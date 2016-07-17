#include "optparse.h"
#include "webserver.h"

#include <cstdio>
#include <unistd.h>


int main(int argc, char **argv) {
    int ret = 0;
    options opts;
    ret = parse_args(argc, argv, opts);
    if (ret < 0) {
        show_usage(argv[0]);
        return 1;
    }

    //Daemonize
    if (opts.daemonize) {
        printf("Info: Daemonize...\n");
        daemon(1, 1);
    }

    //Main work loop
    ret = web_server_run(opts);
    if (ret < 0) {
        return 1;
    }
}