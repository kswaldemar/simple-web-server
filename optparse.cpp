//
// Created by valdemar on 17.07.16.
//
#include "optparse.h"

#include <cstdio>
#include <getopt.h>
#include <string.h>

#include <arpa/inet.h>

const char *OPTIONS_STR = "d:h:p:sw:";

int parse_args(int argc, char **argv, options &opts) {
    int opt = getopt(argc, argv, OPTIONS_STR);
    if (opt == -1) {
        printf("You should specify options!\n");
        return -1;
    }
    uint8_t required_opt = 0;
    while (opt != -1) {
        switch (opt) {
            case 'd':
                strcpy(opts.server_root, optarg);
                required_opt |= 0x1;
                break;
            case 'h':
                if (inet_aton(optarg, &opts.server_ip) == 0) {
                    fprintf(stderr, "Error: '%s' is not valid ipv4 address\n", optarg);
                    return -1;
                }
                required_opt |= 0x2;
                break;
            case 'p':
                if (sscanf(optarg, "%hu", &opts.server_port) != 1) {
                    fprintf(stderr, "Error: '%s' is not valid port value\n", optarg);
                    return -2;
                }
                required_opt |= 0x4;
                break;
            case 's':
                opts.daemonize = false;
                break;
            case 'w':
                if (sscanf(optarg, "%hu", &opts.workers_count) != 1) {
                    fprintf(stderr, "Error: '%s' is not workers count\n", optarg);
                    return -5;
                }
                break;
            case '?':
            default:
                return -3;
        }
        opt = getopt(argc, argv, OPTIONS_STR);
    }
    if (required_opt != 0x7) {
        fprintf(stderr, "Error: missing one of required options!\n");
        return -4;
    }
    return 0;
}

void show_usage(const char *ex_path) {
    printf("%s -h <ip> -p <port> -d <directory> [-s] [-w <num>]\n", ex_path);
    printf("Available options:\n"
               "\t-h host ipv4 address\n"
               "\t-p port in which server should run\n"
               "\t-d web server root directory\n"
               "\t-s stay on foreground (no daemonize). Default: no.\n"
               "\t-w number of worker threads. Default: 4.\n");
}
