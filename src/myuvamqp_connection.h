#ifndef _MYUVAMQP_CONNECTION_H
#define _MYUVAMQP_CONNECTION_H

#include <uv.h>
#include "myuvamqp_buffer.h"
#include "myuvamqp_list.h"
#include "myuvamqp_types.h"
#include "myuvamqp_channel.h"

typedef struct myuvamqp_connection myuvamqp_connection_t;
typedef struct myuvamqp_channel myuvamqp_channel_t;

typedef struct {
    void (*callback)(myuvamqp_connection_t *, void *, int);
    void *arg;
} myuvamqp_connect_cb_t;

typedef struct {
    void (*callback)(myuvamqp_connection_t *, void *);
    void *arg;
} myuvamqp_auth_cb_t;

typedef struct {
    void (*callback)(myuvamqp_connection_t *, void *);
    void *arg;
} myuvamqp_close_cb_t;

struct myuvamqp_connection {
    uv_tcp_t tcp;
    myuvamqp_long_string_t user;
    myuvamqp_long_string_t password;
    myuvamqp_short_string_t vhost;
    int connection_state;
    myuvamqp_list_t *channels;
    myuvamqp_short_t channel_ids;
    myuvamqp_connect_cb_t connect_cb;
    myuvamqp_auth_cb_t auth_cb;
    myuvamqp_close_cb_t close_cb;
};

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
    void (*write_callback)(myuvamqp_connection_t *, void *, int);
    void *arg;
} myuvamqp_write_req_t;


#define MYUVAMQP_STATE_NOT_CONNECTED    1
#define MYUVAMQP_STATE_CONNECTING       2
#define MYUVAMQP_STATE_CONNECTED        3
#define MYUVAMQP_STATE_LOGGEDIN         4
#define MYUVAMQP_STATE_CLOSING          5


#define MYUVAMQP_FRAME_METHOD          1
#define MYUVAMQP_FRAME_HEADER          2
#define MYUVAMQP_FRAME_BODY            3
#define MYUVAMQP_FRAME_HEARTBEAT       4

int myuvamqp_connection_init(uv_loop_t *loop, myuvamqp_connection_t *amqp_conn);
int myuvamqp_connection_start(
    myuvamqp_connection_t *amqp_conn,
    const char *host,
    unsigned short port,
    void (*connect_cb)(myuvamqp_connection_t *, void *, int),
    void *arg);

int myuvamqp_connection_write(
    myuvamqp_connection_t *amqp_conn,
    myuvamqp_buf_helper_t *helper,
    void (*write_cb)(myuvamqp_connection_t *, void *, int),
    void *arg);

int myuvamqp_connection_authenticate(
    myuvamqp_connection_t *amqp_conn,
    myuvamqp_long_string_t user,
    myuvamqp_long_string_t password,
    myuvamqp_short_string_t vhost,
    void (*auth_cb)(myuvamqp_connection_t *, void *),
    void *arg);

myuvamqp_channel_t *myuvamqp_connection_create_channel(myuvamqp_connection_t *amqp_conn);
myuvamqp_channel_t *myuvamqp_connection_find_channel_by_id(myuvamqp_connection_t *amqp_conn, myuvamqp_short_t channel_id);

void myuvamqp_connection_set_close_callback(
    myuvamqp_connection_t *amqp_conn,
    void (*close_cb)(myuvamqp_connection_t *, void *),
    void *arg);

void myuvamqp_connection_close(myuvamqp_connection_t *amqp_conn);
void myuvamqp_connection_remove_channel(myuvamqp_connection_t *amqp_conn, myuvamqp_channel_t *channel);

#endif
