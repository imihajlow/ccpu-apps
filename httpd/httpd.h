#pragma once
#include <stdbool.h>
#include <stdint.h>

#define MAX_URL 31
#define MAX_METHOD 7
#define MAX_MIME 31

enum http_state_t {
    HTTP_METHOD,
    HTTP_URL,
    HTTP_VERSION,
    HTTP_CR, // 13
    HTTP_LF, // 10
    HTTP_HEADER_KEY,
    HTTP_HEADER_VALUE,
    HTTP_WHITESPACE,
    HTTP_BODY,

    HTTP_RSP_HEADER,
    HTTP_DATA,

    HTTP_FINISH,
    HTTP_METHOD_NOT_ALLOWED,
    HTTP_BAD_REQUEST,
    HTTP_NOT_FOUND,
};

struct httpd_appstate_t {
    enum http_state_t state;
    enum http_state_t next_state;
    char method[MAX_METHOD + 1];
    char url[MAX_URL + 1];
    char mime[MAX_MIME + 1];
    char *write_ptr;
    char fd;
    bool done;
    uint32_t last_pos;
};

void httpd_appcall(void);
