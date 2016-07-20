//
// Created by valdemar on 17.07.16.
//

#ifndef SIMPLE_WEB_SERVER_HTTPPARSER_H
#define SIMPLE_WEB_SERVER_HTTPPARSER_H

#include <string>
#include <vector>

struct http_response_msg {
    std::string status_line;
    std::vector<std::string> headers;
    FILE *req_file;
};

int parse_request(char *msg, http_response_msg &response, const char *webserv_root_dir);


#endif //SIMPLE_WEB_SERVER_HTTPPARSER_H
