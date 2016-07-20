#include "optparse.h"
#include "webserver.h"

#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <memory.h>
#include <pthread.h>

extern pthread_cond_t g_condvar;

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

    //Process termination signals
    sigset_t term_signals;
    sigemptyset(&term_signals);
    sigaddset(&term_signals, SIGINT);
    sigaddset(&term_signals, SIGTERM);

    struct sigaction act;
    act.sa_handler = [](int signo) {
        printf("Info: Got signal %s\n", strsignal(signo));
        printf("Info: Trying to exit\n");
        g_web_server_stop = true;
        pthread_cond_broadcast(&g_condvar);
    };
    act.sa_mask = term_signals;
    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);

    //Main work loop
    ret = web_server_run(opts);
    if (ret < 0) {
        return 1;
    }
}