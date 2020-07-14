#include <string.h>
#include <uv.h>
#include "myuvamqp.h"
#include "utils.h"

void cb_connect(myuvamqp_connection_t *connection, void *arg, int err);
void cb_authenticated(myuvamqp_connection_t *connection, void *arg);
void cb_channel_opened(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg);
void cb_exchange_declared(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg);
void cb_exchange_deleted(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg);
void cb_queue_declared(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg);
void cb_queue_deleted(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg);
void cb_queue_bind(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg);
void cb_queue_unbind(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg);
void do_queue_unbind(void *arg);

int connect_cb_called = 0;
int auth_cb_called = 0;
int cb_channel_opened_called = 0;
int cb_exchange_declared_called = 0;
int cb_exchange_deleted_called = 0;
int cb_queue_declared_called = 0;
int cb_queue_deleted_called = 0;
int cb_queue_bind_called = 0;
int cb_queue_unbind_called = 0;

char *queue_name = "myuvamqp-test-queue";
char *exchange_name = "myuvamqp-test-exchange";
char *routing_key = "myuvamqp-test-routing-key";

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
    ASSERT(1 == cb_channel_opened_called);
    ASSERT(1 == cb_exchange_declared_called);
    // ASSERT(1 == cb_exchange_deleted_called);
    ASSERT(1 == cb_queue_declared_called);
    // ASSERT(1 == cb_queue_deleted_called);
    ASSERT(1 == cb_queue_bind_called);
    ASSERT(1 == cb_queue_unbind_called);

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
    int err;
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

    channel = myuvamqp_connection_create_channel(connection);
    ASSERT(0 == channel->channel_closing);
    ASSERT(0 == channel->channel_closed);
    ASSERT(0 == channel->channel_opened);
    err = myuvamqp_channel_open(channel, cb_channel_opened, connection);
    ASSERT(0 == err);
}

void cb_channel_opened(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg)
{
    int err;
    myuvamqp_connection_t *amqp_conn = arg;
    cb_channel_opened_called += 1;

    ASSERT(MYUVAMQP_REPLY_CODE_SUCCESS == reply->reply_code);
    ASSERT(NULL == reply->reply_text);
    ASSERT(MYUVAMQP_CHANNEL_CLASS == reply->class_id);
    ASSERT(MYUVAMQP_CHANNEL_OPEN == reply->method_id);

    ASSERT(channel->connection == amqp_conn);
    ASSERT(0 == channel->channel_closing);
    ASSERT(0 == channel->channel_closed);
    ASSERT(1 == channel->channel_opened);

    myuvamqp_channel_free_reply(reply);

    err = myuvamqp_channel_exchange_declare(
        channel, exchange_name, "fanout", FALSE, FALSE, FALSE,
        FALSE, FALSE, NULL, cb_exchange_declared, channel);

    ASSERT(0 == err);
}


void cb_exchange_declared(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg)
{
    int err;
    myuvamqp_exchange_declare_reply_t *exchange_declare_reply = (myuvamqp_exchange_declare_reply_t *) reply;

    cb_exchange_declared_called += 1;

    ASSERT(arg == channel);
    ASSERT(MYUVAMQP_EXCHANGE_CLASS == exchange_declare_reply->class_id);
    ASSERT(MYUVAMQP_EXCHANGE_DECLARE == exchange_declare_reply->method_id);
    ASSERT(MYUVAMQP_REPLY_CODE_SUCCESS == exchange_declare_reply->reply_code);
    ASSERT(NULL == exchange_declare_reply->reply_text);
    ASSERT(1 == channel->channel_id);
    ASSERT(0 == MYUVAMQP_LIST_LEN(channel->operations[MYUVAMQP_EXCHANGE_DECLARE_OPERATION]));

    myuvamqp_channel_free_reply(reply);

    err = myuvamqp_channel_queue_declare(channel, queue_name, FALSE, FALSE, FALSE, FALSE, FALSE, NULL, cb_queue_declared, channel);
    ASSERT(0 == err);
}

void cb_exchange_deleted(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg)
{
    int err;
    myuvamqp_exchange_delete_reply_t *exchange_delete_reply = (myuvamqp_exchange_delete_reply_t *) reply;

    cb_exchange_deleted_called += 1;

    ASSERT(arg == channel);
    ASSERT(MYUVAMQP_EXCHANGE_CLASS == exchange_delete_reply->class_id);
    ASSERT(MYUVAMQP_EXCHANGE_DELETE == exchange_delete_reply->method_id);
    ASSERT(MYUVAMQP_REPLY_CODE_SUCCESS == exchange_delete_reply->reply_code);
    ASSERT(NULL == exchange_delete_reply->reply_text);
    ASSERT(1 == channel->channel_id);
    ASSERT(0 == MYUVAMQP_LIST_LEN(channel->operations[MYUVAMQP_EXCHANGE_DELETE_OPERATION]));

    myuvamqp_channel_free_reply(reply);
    err = myuvamqp_channel_queue_delete(channel, queue_name, FALSE, FALSE, FALSE, cb_queue_deleted, channel);
    ASSERT(0 == err);
}

void cb_queue_declared(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg)
{
    int err;
    myuvamqp_queue_declare_reply_t *queue_declare_reply = (myuvamqp_queue_declare_reply_t *) reply;

    cb_queue_declared_called += 1;

    ASSERT(arg == channel);
    ASSERT(MYUVAMQP_REPLY_CODE_SUCCESS == queue_declare_reply->reply_code);
    ASSERT(NULL == queue_declare_reply->reply_text);
    ASSERT(MYUVAMQP_QUEUE_CLASS == queue_declare_reply->class_id);
    ASSERT(MYUVAMQP_QUEUE_DECLARE == queue_declare_reply->method_id);
    ASSERT(0 == memcmp(queue_declare_reply->queue, queue_name, strlen(queue_name)));
    ASSERT(0 == queue_declare_reply->message_count);
    ASSERT(0 == queue_declare_reply->consumer_count);
    ASSERT(1 == channel->channel_id);
    ASSERT(0 == MYUVAMQP_LIST_LEN(channel->operations[MYUVAMQP_QUEUE_DECLARE_OPERATION]));

    myuvamqp_channel_free_reply(reply);

    err = myuvamqp_channel_queue_bind(channel, queue_name, exchange_name, routing_key, FALSE, NULL, cb_queue_bind, channel);
    ASSERT(0 == err);
}

void cb_queue_deleted(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg)
{
    myuvamqp_queue_delete_reply_t *queue_delete_reply = (myuvamqp_queue_delete_reply_t *) reply;

    cb_queue_deleted_called += 1;

    ASSERT(arg == channel);
    ASSERT(MYUVAMQP_QUEUE_CLASS == queue_delete_reply->class_id);
    ASSERT(MYUVAMQP_QUEUE_DELETE == queue_delete_reply->method_id);
    ASSERT(MYUVAMQP_REPLY_CODE_SUCCESS == queue_delete_reply->reply_code);
    ASSERT(NULL == queue_delete_reply->reply_text);
    ASSERT(0 == queue_delete_reply->message_count);
    ASSERT(1 == channel->channel_id);
    ASSERT(0 == MYUVAMQP_LIST_LEN(channel->operations[MYUVAMQP_QUEUE_DELETE_OPERATION]));

    myuvamqp_mem_free(reply);

    uv_close((uv_handle_t *) channel->connection, NULL);
    CLEANUP_CONNECTION(channel->connection);
}

void cb_queue_bind(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg)
{
    myuvamqp_queue_bind_reply_t *queue_bind_reply = (myuvamqp_queue_bind_reply_t *) reply;

    cb_queue_bind_called += 1;

    ASSERT(arg == channel);
    ASSERT(MYUVAMQP_QUEUE_CLASS == queue_bind_reply->class_id);
    ASSERT(MYUVAMQP_QUEUE_BIND == queue_bind_reply->method_id);
    ASSERT(MYUVAMQP_REPLY_CODE_SUCCESS == queue_bind_reply->reply_code);
    ASSERT(NULL == queue_bind_reply->reply_text);
    ASSERT(1 == channel->channel_id);
    ASSERT(0 == MYUVAMQP_LIST_LEN(channel->operations[MYUVAMQP_QUEUE_BIND_OPERATION]));

    myuvamqp_mem_free(reply);
    call_later(UV_LOOP(channel->connection), 500, do_queue_unbind, channel);
}

void cb_queue_unbind(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg)
{
    int err;
    myuvamqp_queue_unbind_reply_t *queue_unbind_reply = (myuvamqp_queue_unbind_reply_t *) reply;

    cb_queue_unbind_called += 1;

    ASSERT(arg == channel);
    ASSERT(MYUVAMQP_QUEUE_CLASS == queue_unbind_reply->class_id);
    ASSERT(MYUVAMQP_QUEUE_UNBIND == queue_unbind_reply->method_id);
    ASSERT(MYUVAMQP_REPLY_CODE_SUCCESS == queue_unbind_reply->reply_code);
    ASSERT(NULL == queue_unbind_reply->reply_text);
    ASSERT(1 == channel->channel_id);
    ASSERT(0 == MYUVAMQP_LIST_LEN(channel->operations[MYUVAMQP_QUEUE_UNBIND_OPERATION]));

    myuvamqp_mem_free(reply);
    err = myuvamqp_channel_exchange_delete(channel, exchange_name, FALSE, FALSE, cb_exchange_deleted, channel);
    ASSERT(0 == err);
}

void do_queue_unbind(void *arg)
{
    int err;
    myuvamqp_channel_t *channel = arg;

    err = myuvamqp_channel_queue_unbind(channel, queue_name, exchange_name, routing_key, NULL, cb_queue_unbind, channel);
    ASSERT(0 == err);
}
