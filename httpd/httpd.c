#include "httpd.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <libsys/vga.h>
#include <libsys/fat/fat.h>

#include "uip.h"

#define RESOURCE_FOLDER "/WWW"
#define CHUNK_SIZE UIP_APPDATA_SIZE

static void process_data(struct httpd_appstate_t *s);
static void method_not_allowed(struct httpd_appstate_t *s);
static void bad_request(struct httpd_appstate_t *s);
static void not_found(struct httpd_appstate_t *s);
static void open_resource(struct httpd_appstate_t *s);
static void rsp_header(struct httpd_appstate_t *s);
static void fill_mime(struct httpd_appstate_t *s, const char *filename);
static void rexmit_data(struct httpd_appstate_t *s);
static void send_data(struct httpd_appstate_t *s);


void httpd_appcall(void) {
    struct httpd_appstate_t *s = (struct httpd_appstate_t *)&(uip_conn->appstate);
    if (uip_connected()) {
        s->state = HTTP_METHOD;
        s->write_ptr = s->method;
        s->done = false;
    } else if (uip_closed() || uip_aborted() || uip_timedout()) {
        switch (s->state) {
        case HTTP_RSP_HEADER:
        case HTTP_DATA:
            fat_close(s->fd);
            break;
        }
        s->state = HTTP_FINISH;
        return;
    }
    if (uip_newdata()) {
        process_data(s);
    }
    if (uip_acked()) {
        if (s->state >= HTTP_FINISH) {
            uip_close();
        } else switch (s->state) {
        case HTTP_RSP_HEADER:
        case HTTP_DATA:
            if (s->done) {
                fat_close(s->fd);
                uip_close();
                s->state = HTTP_FINISH;
            } else {
                s->state = HTTP_DATA;
                send_data(s);
            }
            break;
        }
    } else if (uip_rexmit()) {
        switch (s->state) {
        case HTTP_METHOD_NOT_ALLOWED:
            method_not_allowed(s);
            break;
        case HTTP_BAD_REQUEST:
            bad_request(s);
            break;
        case HTTP_NOT_FOUND:
            not_found(s);
            break;
        case HTTP_RSP_HEADER:
            rsp_header(s);
            break;
        case HTTP_DATA:
            rexmit_data(s);
            break;
        }
    }

}

static void process_data(struct httpd_appstate_t *s) {
    size_t len = uip_datalen();
    char *dataptr = (char *)uip_appdata;
    while (len) {
        char c = *dataptr;
        switch (s->state) {
        case HTTP_METHOD:
            if (c == ' ') {
                *s->write_ptr = 0;
                s->state = HTTP_WHITESPACE;
                s->next_state = HTTP_URL;
                s->write_ptr = s->url;
                if (strcmp(s->method, "GET") != 0) {
                    method_not_allowed(s);
                    return;
                }
            } else {
                *s->write_ptr = c;
                s->write_ptr += 1;
                if (s->write_ptr - (char*)s->method > MAX_METHOD) {
                    bad_request(s);
                    return;
                }
            }
            break;
        case HTTP_URL:
            if (c == ' ') {
                *s->write_ptr = 0;
                s->state = HTTP_WHITESPACE;
                s->next_state = HTTP_VERSION;
            } else {
                *s->write_ptr = c;
                s->write_ptr += 1;
                if (s->write_ptr - (char*)s->url > MAX_URL) {
                    bad_request(s);
                    return;
                }
            }
            break;
        case HTTP_VERSION:
            if (c == 13) {
                s->state = HTTP_CR;
                s->next_state = HTTP_HEADER_KEY;
            }
            break;
        case HTTP_CR:
            if (c == 10) {
                s->state = HTTP_LF;
            } else {
                bad_request(s);
                return;
            }
            break;
        case HTTP_LF:
            if (c == 13) {
                s->state = HTTP_CR;
                s->next_state = HTTP_BODY;
                open_resource(s);
                return;
            } else {
                s->state = s->next_state;
            }
            break;
        case HTTP_HEADER_KEY:
            if (c == ':') {
                s->state = HTTP_WHITESPACE;
                s->next_state = HTTP_HEADER_VALUE;
            }
            break;
        case HTTP_HEADER_VALUE:
            if (c == 13) {
                s->state = HTTP_CR;
                s->next_state = HTTP_HEADER_KEY;
            }
            break;
        case HTTP_WHITESPACE:
            if (c != ' ') {
                s->state = s->next_state;
                dataptr -= 1;
                len += 1;
            }
            break;
        case HTTP_BODY:
            return;
        default:
            return;
        }
        len -= 1;
        dataptr += 1;
    }
}

static void method_not_allowed(struct httpd_appstate_t *s) {
    uip_send("HTTP/1.1 405 Method Not Allowed\r\n", 33);

    s->state = HTTP_METHOD_NOT_ALLOWED;
}

static void bad_request(struct httpd_appstate_t *s) {
    uip_send("HTTP/1.1 400 Bad Request\r\n", 26);
    s->state = HTTP_BAD_REQUEST;
}

static void not_found(struct httpd_appstate_t *s) {
    uip_send("HTTP/1.1 404 Not Found\r\n", 24);
    s->state = HTTP_NOT_FOUND;
}

static void abort_connection(struct httpd_appstate_t *s) {
    switch (s->state) {
    case HTTP_RSP_HEADER:
    case HTTP_DATA:
        fat_close(s->fd);
        break;
    }
    uip_abort();
}

static char filename_buf[MAX_URL + 12] __attribute__((section("bss_hi")));
static void open_resource(struct httpd_appstate_t *s) {
    if (s->url[0] != '/') {
        not_found(s);
        return;
    }
    if (strcmp(s->url, "/") == 0) {
        strcpy(s->url, "/INDEX.HTM");
    }
    strcpy(filename_buf, RESOURCE_FOLDER);
    strcat(filename_buf, s->url);
    puts(filename_buf);
    uint8_t fd = fat_open_path(filename_buf, 0);
    if (fd == FAT_BAD_DESC) {
        fat_print_last_error();
        not_found(s);
        return;
    }
    puts("OK");
    s->fd = fd;
    s->last_pos = 0;
    s->state = HTTP_RSP_HEADER;
    fill_mime(s, filename_buf);
    rsp_header(s);
}

static void rsp_header(struct httpd_appstate_t *s) {
    char *buf = &uip_buf[UIP_IPTCPH_LEN + UIP_LLH_LEN];
    strcpy(buf, "HTTP/1.1 200 OK\r\nContent-Type: ");
    strcat(buf, s->mime);
    strcat(buf, "\r\n\r\n");
    uip_send(buf, strlen(buf));
}

static void send_data(struct httpd_appstate_t *s) {
    s->last_pos = fat_tell(s->fd);
    char *buf = &uip_buf[UIP_IPTCPH_LEN + UIP_LLH_LEN];
    size_t len = fat_read(s->fd, buf, CHUNK_SIZE);
    if (len < CHUNK_SIZE) {
        s->done = true;
    }
    uip_send(buf, len);
}

static void rexmit_data(struct httpd_appstate_t *s) {
    if (fat_seek(s->fd, s->last_pos)) {
        send_data(s);
    } else {
        abort_connection(s);
    }
}

enum mime_t {
    MIME_UNKNOWN,
    MIME_HTML,
    MIME_CSS,
    MIME_JS,
    MIME_PNG,
    MIME_JPG,
};

static void fill_mime(struct httpd_appstate_t *s, const char *filename) {
    char *p = strrchr(filename, '.');
    enum mime_t mime = MIME_UNKNOWN;
    if (p) {
        p += 1;
        if (strcmp(p, "HTM") == 0) {
            mime = MIME_HTML;
        } else if (strcmp(p, "CSS") == 0) {
            mime = MIME_CSS;
        } else if (strcmp(p, "JS") == 0) {
            mime = MIME_JS;
        } else if (strcmp(p, "PNG") == 0) {
            mime = MIME_PNG;
        } else if (strcmp(p, "JPG") == 0) {
            mime = MIME_JPG;
        }
    }
    switch (mime) {
        case MIME_UNKNOWN:
            strcpy(s->mime, "application/octet-stream");
            break;
        case MIME_HTML:
            strcpy(s->mime, "text/html");
            break;
        case MIME_CSS:
            strcpy(s->mime, "text/css");
            break;
        case MIME_JS:
            strcpy(s->mime, "text/javascript");
            break;
        case MIME_PNG:
            strcpy(s->mime, "image/png");
            break;
        case MIME_JPG:
            strcpy(s->mime, "image/jpeg");
            break;
    }
}
