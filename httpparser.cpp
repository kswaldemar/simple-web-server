//
// Created by valdemar on 17.07.16.
//

#include "httpparser.h"

#include <cstring>

int parse_request(char *msg, http_response_msg &response, const char *webserv_root_dir) {
    response.req_file = nullptr;
    response.headers.clear();

    char *pos = strtok(msg, " ");
    std::string method = pos;

    std::string filepath = webserv_root_dir;
    filepath += strtok(nullptr, " ");

    //Get params support
    unsigned long idx = filepath.find('?');
    if (idx != std::string::npos) {
        filepath[idx] = '\0';
    }

    std::string http_ver = strtok(nullptr, "\n");
    http_ver.pop_back();
    if (http_ver.back() == '\r') {
        http_ver.pop_back();
    }

    FILE *file = fopen(filepath.c_str(), "r");
    response.req_file = file;

    //printf("Info: Http=%s, Method=%s, URI=%s, File=%s\n",
    //       http_ver.c_str(), method.c_str(), filepath.c_str(),
    //       file ? "Exists" : "Not found");

    response.status_line = http_ver;
    response.status_line += " ";

    if (file) {
        response.status_line += "200 Ok";
        fseek(file, 0, SEEK_END);
        long byte_size = ftell(file);
        response.headers.emplace_back("Content-Length: " + std::to_string(byte_size));
        rewind(file);
    } else {
        response.status_line += "404 Not found";
        response.headers.emplace_back("Content-Length: 0");
    }
    return 0;
}
