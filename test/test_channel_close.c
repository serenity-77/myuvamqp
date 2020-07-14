#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "myuvamqp.h"
#include "utils.h"

#define CHANNEL_MAX 5

void cb_connect(myuvamqp_connection_t *connection, void *arg, int err);
void cb_authenticated(myuvamqp_connection_t *connection, void *arg);
void cb_channel_opened(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg);
void cb_channel_closed(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg);
void do_close_channels(uv_timer_t *timer);
void cb_timer_closed(uv_handle_t *timer);

int connect_cb_called = 0;
int auth_cb_called = 0;
int cb_channel_opened_called = 0;
int channel_id_sum = 0;
int cb_channel_closed_called = 0;
int channel_closing_idx = 0;
myuvamqp_channel_t *closing_channels[2];

int main(int argc, char *argv[])
{
    int err;
    uv_loop_t loop;
    myuvamqp_connection_t amqp_conn;

    err = uv_loop_init(&loop);
    ABORT_IF_ERROR(err);

    err = myuvamqp_connection_init(&loop, &amqp_conn);
    ABORT_IF_ERROR(err);
    ASSERT(NULL == amqp_conn.user);
    ASSERT(NULL == amqp_conn.password);
    ASSERT(NULL == amqp_conn.vhost);
    ASSERT(NULL == amqp_conn.connect_cb.callback);
    ASSERT(NULL == amqp_conn.auth_cb.callback);
    ASSERT(NULL != amqp_conn.channels);
    ASSERT(MYUVAMQP_STATE_NOT_CONNECTED == amqp_conn.connection_state);

    err = myuvamqp_connection_start(&amqp_conn, "127.0.0.1", 5672, cb_connect, NULL);
    ABORT_IF_ERROR(err);
    ASSERT(MYUVAMQP_STATE_CONNECTING == amqp_conn.connection_state);

    ASSERT(0 == err);

    uv_run(&loop, UV_RUN_DEFAULT);

    ASSERT(1 == connect_cb_called);
    ASSERT(1 == auth_cb_called);
    ASSERT(CHANNEL_MAX == cb_channel_opened_called);
    ASSERT((1 + 2 + 3 + 4 + 5) == channel_id_sum);
    ASSERT(2 == cb_channel_closed_called);

    uv_loop_close(&loop);

    return 0;
}


void cb_connect(myuvamqp_connection_t *amqp_conn, void *arg, int err)
{
    connect_cb_called += 1;
    ASSERT(MYUVAMQP_STATE_CONNECTED == amqp_conn->connection_state);
    ASSERT(0 == err);
    ASSERT(NULL == arg);
    err = myuvamqp_connection_authenticate(amqp_conn, "guest", "guest", "/", cb_authenticated, NULL);
    ASSERT(0 == err);
    ASSERT(0 == memcmp(amqp_conn->user, "guest", strlen("guest")));
    ASSERT(0 == memcmp(amqp_conn->password, "guest", strlen("guest")));
    ASSERT(0 == memcmp(amqp_conn->vhost, "/", strlen("/")));
    ASSERT(cb_authenticated == amqp_conn->auth_cb.callback);
}

void cb_authenticated(myuvamqp_connection_t *connection, void *arg)
{
    int i, err;
    myuvamqp_channel_t *channel;

    channel = connection->channels->head->value;

    auth_cb_called += 1;
    ASSERT(NULL == arg);
    ASSERT(MYUVAMQP_STATE_LOGGEDIN == connection->connection_state);
    ASSERT(1 == connection->channels->len); // channel0
    ASSERT(1 == connection->channel_ids);
    ASSERT(1 == channel->channel_opened);
    ASSERT(0 == channel->channel_closing);
    ASSERT(0 == channel->channel_closed);

    for(i = 0; i < CHANNEL_MAX; i++)
    {
        channel = myuvamqp_connection_create_channel(connection);
        ASSERT(0 == channel->channel_closing);
        ASSERT(0 == channel->channel_closed);
        ASSERT(0 == channel->channel_opened);
        err = myuvamqp_channel_open(channel, cb_channel_opened, connection);
        ASSERT(0 == err);
    }
}

void cb_channel_opened(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg)
{
    myuvamqp_connection_t *amqp_conn = arg;
    cb_channel_opened_called += 1;
    channel_id_sum += channel->channel_id;

    ASSERT(MYUVAMQP_REPLY_CODE_SUCCESS == reply->reply_code);
    ASSERT(NULL == reply->reply_text);
    ASSERT(MYUVAMQP_CHANNEL_CLASS == reply->class_id);
    ASSERT(MYUVAMQP_CHANNEL_OPEN == reply->method_id);

    ASSERT(channel->connection == amqp_conn);
    ASSERT(0 == channel->channel_closing);
    ASSERT(0 == channel->channel_closed);
    ASSERT(1 == channel->channel_opened);

    myuvamqp_channel_free_reply(reply);

    if(channel->channel_id == 2 || channel->channel_id == 4)
    {
        closing_channels[channel_closing_idx++] = channel;
    }

    if(cb_channel_opened_called == CHANNEL_MAX)
    {
        ASSERT((CHANNEL_MAX + 1) == MYUVAMQP_LIST_LEN(channel->connection->channels));

        uv_timer_t *timer = myuvamqp_mem_alloc(sizeof(*timer));
        uv_timer_init(((uv_handle_t *) channel->connection)->loop, timer);
        uv_timer_start(timer, do_close_channels, 500, 0);
    }
}

void cb_channel_closed(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg)
{
    myuvamqp_connection_t *connection = channel->connection;

    cb_channel_closed_called += 1;

    ASSERT(NULL == arg);
    ASSERT(MYUVAMQP_REPLY_CODE_SUCCESS == reply->reply_code);
    ASSERT(NULL == reply->reply_text);
    ASSERT(MYUVAMQP_CHANNEL_CLASS == reply->class_id);
    ASSERT(MYUVAMQP_CHANNEL_CLOSE == reply->method_id);
    ASSERT(0 == channel->channel_opened);
    ASSERT(1 == channel->channel_closed);
    ASSERT(0 == channel->channel_closing);

    myuvamqp_connection_remove_channel(channel->connection, channel);

    ASSERT(((CHANNEL_MAX + 1) - cb_channel_closed_called) == MYUVAMQP_LIST_LEN(connection->channels));

    myuvamqp_channel_free_reply(reply);

    if(cb_channel_closed_called == 2)
    {
        uv_close((uv_handle_t *) connection, NULL);
        CLEANUP_CONNECTION(connection); // make valgrind happy
    }
}

void do_close_channels(uv_timer_t *timer)
{
    int i = 0, err;

    uv_close((uv_handle_t *) timer, cb_timer_closed);

    for(i = 0; i < sizeof(closing_channels) / sizeof(closing_channels[0]); i++)
    {
        err = myuvamqp_channel_close(closing_channels[i], 0, NULL, 0, 0, cb_channel_closed, NULL);
        ASSERT(0 == err);
        ASSERT(1 == closing_channels[i]->channel_closing);
    }
}

void cb_timer_closed(uv_handle_t *timer)
{
    myuvamqp_mem_free(timer);
}
