#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "myuvamqp_frame.h"
#include "myuvamqp_mem.h"
#include "myuvamqp_channel.h"
#include "myuvamqp_content.h"

#define MYUVAMQP_REMOVE_OPERATION(channel, operation)    \
    do {    \
        if(operation)   \
            myuvamqp_list_remove_by_value((channel)->operations[(operation)->operation_type], (operation));  \
    } while(0);

#define MYUVAMQP_GET_OPERATION(channel, operation, operation_type) \
    do {    \
        memcpy(&operation, MYUVAMQP_LIST_HEAD_VALUE(channel->operations[operation_type]), sizeof(operation));   \
        myuvamqp_list_remove_head(channel->operations[operation_type]);    \
    } while(0)


static void myuvamqp_channel_reply_init(
    myuvamqp_reply_t *reply,
    myuvamqp_short_t reply_code,
    myuvamqp_short_string_t reply_text,
    myuvamqp_short_t class_id,
    myuvamqp_short_t method_id);

static myuvamqp_operation_t *myuvamqp_channel_add_operation(
    myuvamqp_channel_t *channel,
    int operation_type,
    void (*callback)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg);

static void myuvamqp_free_operation(void *arg);
static int myuvamqp_get_operation_type(myuvamqp_short_t class_id, myuvamqp_short_t method_id);
static void *myuvamqp_reply_create(int operation_type);

static int myuvamqp_channel_handle_reply(
    myuvamqp_channel_t *channel,
    myuvamqp_short_t reply_code,
    myuvamqp_short_string_t reply_text,
    myuvamqp_short_t class_id,
    myuvamqp_short_t method_id);


int myuvamqp_channel_init(myuvamqp_channel_t *channel, myuvamqp_connection_t *amqp_conn, myuvamqp_short_t channel_id)
{
    channel->channel_id = channel_id;
    channel->connection = amqp_conn;

    if(!channel_id)
        channel->channel_opened = 1;
    else
        channel->channel_opened = 0;

    channel->channel_closed = 0;
    channel->channel_closing = 0;

    channel->cb_open.callback = NULL;
    channel->cb_open.arg = NULL;
    channel->cb_close.callback = NULL;
    channel->cb_close.arg = NULL;

    memset(channel->operations, 0, sizeof(channel->operations));

    return 0;
}

int myuvamqp_channel_open(
    myuvamqp_channel_t *channel,
    void (*cb_open)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    // allow reopening the channel
    if(channel->channel_closed)
        channel->channel_closed = 0;

    channel->cb_open.callback = cb_open;
    channel->cb_open.arg = arg;

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_CHANNEL_CLASS, MYUVAMQP_CHANNEL_OPEN)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_channel_open(&helper)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    channel->cb_open.callback = NULL;
    channel->cb_open.arg = NULL;
    return err;
}

int myuvamqp_channel_open_ok(myuvamqp_channel_t *channel)
{
    myuvamqp_reply_t *reply;

    channel->channel_opened = 1;

    if(channel->cb_open.callback)
    {
        if(!(reply = myuvamqp_mem_alloc(sizeof(*reply))))
            return -1;
        myuvamqp_channel_reply_init(reply, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_CHANNEL_CLASS, MYUVAMQP_CHANNEL_OPEN);
        channel->cb_open.callback(channel, reply, channel->cb_open.arg);
    }

    return 0;
}

void myuvamqp_channel_free_reply(myuvamqp_reply_t *reply)
{
    int operation_type;

    operation_type = myuvamqp_get_operation_type(reply->class_id, reply->method_id);

    switch(operation_type)
    {
        case MYUVAMQP_QUEUE_DECLARE_OPERATION:
            myuvamqp_mem_free(((myuvamqp_queue_declare_reply_t *) reply)->queue);
            break;

        case MYUVAMQP_BASIC_CONSUME_OPERATION:
            myuvamqp_mem_free(((myuvamqp_basic_consume_reply_t *) reply)->consumer_tag);
            break;
    }

    myuvamqp_mem_free(reply->reply_text);
    myuvamqp_mem_free(reply);
}

void myuvamqp_channel_destroy(myuvamqp_channel_t *channel)
{
    int i;

    for(i = 0; i < sizeof(channel->operations) / sizeof(channel->operations[0]); i++)
        myuvamqp_list_destroy(channel->operations[i]);

    myuvamqp_mem_free(channel);
}

int myuvamqp_channel_queue_declare(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t queue,
    myuvamqp_bit_t passive,
    myuvamqp_bit_t durable,
    myuvamqp_bit_t exclusive,
    myuvamqp_bit_t auto_delete,
    myuvamqp_bit_t no_wait,
    myuvamqp_field_table_t *arguments,
    void (*queue_declare_cb)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_octet_t option = 0;
    myuvamqp_operation_t *operation;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if(!(operation = myuvamqp_channel_add_operation(channel, MYUVAMQP_QUEUE_DECLARE_OPERATION, queue_declare_cb, arg)))
        return -1;

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_QUEUE_CLASS, MYUVAMQP_QUEUE_DECLARE)) < 0)
        goto cleanup;

    option |= passive << 0;
    option |= durable << 1;
    option |= exclusive << 2;
    option |= auto_delete << 3;
    option |= no_wait << 4;

    if((err = myuvamqp_frame_encode_queue_declare(&helper, queue, option, arguments)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    MYUVAMQP_REMOVE_OPERATION(channel, operation)
    return err;
}

int myuvamqp_channel_queue_declare_ok(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t queue,
    myuvamqp_long_t message_count,
    myuvamqp_long_t consumer_count)
{
    myuvamqp_queue_declare_reply_t *reply;
    myuvamqp_operation_t operation;

    MYUVAMQP_GET_OPERATION(channel, operation, MYUVAMQP_QUEUE_DECLARE_OPERATION);

    if(operation.callback)
    {
        if(!(reply = myuvamqp_reply_create(MYUVAMQP_QUEUE_DECLARE_OPERATION)))
            return -1;
        myuvamqp_channel_reply_init((myuvamqp_reply_t *) reply, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_QUEUE_CLASS, MYUVAMQP_QUEUE_DECLARE);
        reply->queue = queue;
        reply->message_count = message_count;
        reply->consumer_count = consumer_count;
        operation.callback(channel, (myuvamqp_reply_t *) reply, operation.arg);
    }

    return 0;
}

int myuvamqp_channel_queue_delete(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t queue,
    myuvamqp_bit_t if_unused,
    myuvamqp_bit_t if_empty,
    myuvamqp_bit_t no_wait,
    void (*queue_delete_cb)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_octet_t option = 0;
    myuvamqp_operation_t *operation;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if(!(operation = myuvamqp_channel_add_operation(channel, MYUVAMQP_QUEUE_DELETE_OPERATION, queue_delete_cb, arg)))
        return -1;

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_QUEUE_CLASS, MYUVAMQP_QUEUE_DELETE)) < 0)
        goto cleanup;

    option |= if_unused << 0;
    option |= if_empty << 1;
    option |= no_wait << 2;

    if((err = myuvamqp_frame_encode_queue_delete(&helper, queue, option)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    MYUVAMQP_REMOVE_OPERATION(channel, operation)
    return err;
}


int myuvamqp_channel_queue_delete_ok(myuvamqp_channel_t *channel, myuvamqp_long_t message_count)
{
    myuvamqp_queue_delete_reply_t *reply;
    myuvamqp_operation_t operation;

    MYUVAMQP_GET_OPERATION(channel, operation, MYUVAMQP_QUEUE_DELETE_OPERATION);

    if(operation.callback)
    {
        if(!(reply = myuvamqp_reply_create(MYUVAMQP_QUEUE_DELETE_OPERATION)))
            return -1;
        myuvamqp_channel_reply_init((myuvamqp_reply_t *) reply, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_QUEUE_CLASS, MYUVAMQP_QUEUE_DELETE);
        reply->message_count = message_count;
        operation.callback(channel, (myuvamqp_reply_t *) reply, operation.arg);
    }

    return 0;
}

int myuvamqp_channel_exchange_declare(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t exchange_type,
    myuvamqp_bit_t passive,
    myuvamqp_bit_t durable,
    myuvamqp_bit_t auto_delete,
    myuvamqp_bit_t internal,
    myuvamqp_bit_t no_wait,
    myuvamqp_field_table_t *arguments,
    void (*exchange_declare_cb)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_octet_t option = 0;
    myuvamqp_operation_t *operation;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if(!(operation = myuvamqp_channel_add_operation(channel, MYUVAMQP_EXCHANGE_DECLARE_OPERATION, exchange_declare_cb, arg)))
        return -1;

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_EXCHANGE_CLASS, MYUVAMQP_EXCHANGE_DECLARE)) < 0)
        goto cleanup;

    option |= passive << 0;
    option |= durable << 1;
    option |= auto_delete << 2;
    option |= internal << 3;
    option |= no_wait << 4;

    if((err = myuvamqp_frame_encode_exchange_declare(&helper, exchange, exchange_type, option, arguments)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    MYUVAMQP_REMOVE_OPERATION(channel, operation)
    return err;
}

int myuvamqp_channel_exchange_declare_ok(myuvamqp_channel_t *channel)
{
    return myuvamqp_channel_handle_reply(channel, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_EXCHANGE_CLASS, MYUVAMQP_EXCHANGE_DECLARE);
}

int myuvamqp_channel_exchange_delete(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t exchange,
    myuvamqp_bit_t if_unused,
    myuvamqp_bit_t no_wait,
    void (*exchange_delete_cb)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_octet_t option = 0;
    myuvamqp_operation_t *operation;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if(!(operation = myuvamqp_channel_add_operation(channel, MYUVAMQP_EXCHANGE_DELETE_OPERATION, exchange_delete_cb, arg)))
        return -1;

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_EXCHANGE_CLASS, MYUVAMQP_EXCHANGE_DELETE)) < 0)
        goto cleanup;

    option |= if_unused << 0;
    option |= no_wait << 1;

    if((err = myuvamqp_frame_encode_exchange_delete(&helper, exchange, option)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    MYUVAMQP_REMOVE_OPERATION(channel, operation)
    return err;
}

int myuvamqp_channel_exchange_delete_ok(myuvamqp_channel_t *channel)
{
    return myuvamqp_channel_handle_reply(channel, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_EXCHANGE_CLASS, MYUVAMQP_EXCHANGE_DELETE);
}

int myuvamqp_channel_close(
    myuvamqp_channel_t *channel,
    myuvamqp_short_t reply_code,
    myuvamqp_short_string_t reply_text,
    myuvamqp_short_t class_id,
    myuvamqp_short_t method_id,
    void (*cb_close)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_buf_helper_t helper = {NULL, 0};
    myuvamqp_reply_t *reply;
    myuvamqp_operation_t operation;
    int operation_type;

    if(!channel->channel_closing)
    {
        channel->channel_closing = 1;
        channel->cb_close.callback = cb_close;
        channel->cb_close.arg = arg;

        if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
            goto cleanup;

        if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_CHANNEL_CLASS, MYUVAMQP_CHANNEL_CLOSE)) < 0)
            goto cleanup;

        if((err = myuvamqp_frame_encode_channel_close(&helper, reply_code, reply_text, class_id, method_id)) < 0)
            goto cleanup;

        if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
            goto cleanup;
    }
    else
    {
        // server close the channel (mostly caused by a channel exception)
        // send channel_close_ok frame.

        channel->channel_closed = 1;
        if((err = myuvamqp_channel_close_ok(channel)) < 0)
            return err;

        if((err = myuvamqp_channel_handle_reply(channel, reply_code, reply_text, class_id, method_id)) < 0)
            return err;
    }

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    channel->cb_close.callback = NULL;
    channel->cb_close.arg = NULL;
    return err;
}

int myuvamqp_channel_close_ok(myuvamqp_channel_t *channel)
{
    int err;
    myuvamqp_reply_t *reply;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    channel->channel_opened = 0;
    channel->channel_closing = 0;

    if(!channel->channel_closed)
    {
        channel->channel_closed = 1;
        if(channel->cb_close.callback)
        {
            if(!(reply = myuvamqp_mem_alloc(sizeof(*reply))))
                return -1;
            myuvamqp_channel_reply_init(reply, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_CHANNEL_CLASS, MYUVAMQP_CHANNEL_CLOSE);
            channel->cb_close.callback(channel, reply, channel->cb_close.arg);
        }
    }
    else
    {
        if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
            goto cleanup;

        if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_CHANNEL_CLASS, MYUVAMQP_CHANNEL_CLOSE_OK)) < 0)
            goto cleanup;

        if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
            goto cleanup;
    }

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    return err;
}


int myuvamqp_channel_queue_bind(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t queue,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t routing_key,
    myuvamqp_bit_t no_wait,
    myuvamqp_field_table_t *arguments,
    void (*cb_bind)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_octet_t option = 0;
    myuvamqp_operation_t *operation;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if(!(operation = myuvamqp_channel_add_operation(channel, MYUVAMQP_QUEUE_BIND_OPERATION, cb_bind, arg)))
        return -1;

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_QUEUE_CLASS, MYUVAMQP_QUEUE_BIND)) < 0)
        goto cleanup;

    option = no_wait;

    if((err = myuvamqp_frame_encode_queue_bind(&helper, queue, exchange, routing_key, option, arguments)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    MYUVAMQP_REMOVE_OPERATION(channel, operation)
    return err;
}

int myuvamqp_channel_queue_bind_ok(myuvamqp_channel_t *channel)
{
    return myuvamqp_channel_handle_reply(channel, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_QUEUE_CLASS, MYUVAMQP_QUEUE_BIND);
}

int myuvamqp_channel_queue_unbind(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t queue,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t routing_key,
    myuvamqp_field_table_t *arguments,
    void (*cb_bind)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_octet_t option = 0;
    myuvamqp_operation_t *operation;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if(!(operation = myuvamqp_channel_add_operation(channel, MYUVAMQP_QUEUE_UNBIND_OPERATION, cb_bind, arg)))
        return -1;

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_QUEUE_CLASS, MYUVAMQP_QUEUE_UNBIND)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_queue_unbind(&helper, queue, exchange, routing_key, arguments)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    MYUVAMQP_REMOVE_OPERATION(channel, operation)
    return err;
}

int myuvamqp_channel_queue_unbind_ok(myuvamqp_channel_t *channel)
{
    return myuvamqp_channel_handle_reply(channel, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_QUEUE_CLASS, MYUVAMQP_QUEUE_UNBIND);
}

int myuvamqp_channel_queue_purge(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t queue,
    myuvamqp_bit_t no_wait,
    void (*cb_purge)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_octet_t option = 0;
    myuvamqp_operation_t *operation;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if(!(operation = myuvamqp_channel_add_operation(channel, MYUVAMQP_QUEUE_PURGE_OPERATION, cb_purge, arg)))
        return -1;

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_QUEUE_CLASS, MYUVAMQP_QUEUE_PURGE)) < 0)
        goto cleanup;

    option = no_wait;

    if((err = myuvamqp_frame_encode_queue_purge(&helper, queue, option)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    MYUVAMQP_REMOVE_OPERATION(channel, operation)
    return err;
}

int myuvamqp_channel_queue_purge_ok(myuvamqp_channel_t *channel, myuvamqp_long_t message_count)
{
    myuvamqp_queue_purge_reply_t *reply;
    myuvamqp_operation_t operation;

    MYUVAMQP_GET_OPERATION(channel, operation, MYUVAMQP_QUEUE_PURGE_OPERATION);

    if(operation.callback)
    {
        if(!(reply = myuvamqp_reply_create(MYUVAMQP_QUEUE_DELETE_OPERATION)))
            return -1;
        myuvamqp_channel_reply_init((myuvamqp_reply_t *) reply, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_QUEUE_CLASS, MYUVAMQP_QUEUE_PURGE);
        reply->message_count = message_count;
        operation.callback(channel, (myuvamqp_reply_t *) reply, operation.arg);
    }

    return 0;
}

int myuvamqp_channel_basic_qos(
    myuvamqp_channel_t *channel,
    myuvamqp_long_t prefetch_size,
    myuvamqp_short_t prefetch_count,
    myuvamqp_bit_t global,
    void (*cb_basic_qos)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_octet_t option = 0;
    myuvamqp_operation_t *operation;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if(!(operation = myuvamqp_channel_add_operation(channel, MYUVAMQP_BASIC_QOS_OPERATION, cb_basic_qos, arg)))
        return -1;

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_BASIC_CLASS, MYUVAMQP_BASIC_QOS)) < 0)
        goto cleanup;

    option = global;

    if((err = myuvamqp_frame_encode_basic_qos(&helper, prefetch_size, prefetch_count, option)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    MYUVAMQP_REMOVE_OPERATION(channel, operation)
    return err;
}

int myuvamqp_channel_basic_qos_ok(myuvamqp_channel_t *channel)
{
    return myuvamqp_channel_handle_reply(channel, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_BASIC_CLASS, MYUVAMQP_BASIC_QOS);
}

int myuvamqp_channel_basic_consume(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t queue,
    myuvamqp_short_string_t consumer_tag,
    myuvamqp_bit_t no_local,
    myuvamqp_bit_t no_ack,
    myuvamqp_bit_t exclusive,
    myuvamqp_bit_t no_wait,
    myuvamqp_field_table_t *arguments,
    void (*cb_basic_consume)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_octet_t option = 0;
    myuvamqp_operation_t *operation;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if(!(operation = myuvamqp_channel_add_operation(channel, MYUVAMQP_BASIC_CONSUME_OPERATION, cb_basic_consume, arg)))
        return -1;

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_BASIC_CLASS, MYUVAMQP_BASIC_CONSUME)) < 0)
        goto cleanup;

    option |= no_local << 0;
    option |= no_ack << 1;
    option |= exclusive << 2;
    option |= no_wait << 3;

    if((err = myuvamqp_frame_encode_basic_consume(&helper, queue, consumer_tag, option, arguments)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    MYUVAMQP_REMOVE_OPERATION(channel, operation)
    return err;
}


int myuvamqp_channel_basic_consume_ok(myuvamqp_channel_t *channel, myuvamqp_short_string_t consumer_tag)
{
    myuvamqp_basic_consume_reply_t *reply;
    myuvamqp_operation_t operation;

    MYUVAMQP_GET_OPERATION(channel, operation, MYUVAMQP_BASIC_CONSUME_OPERATION);

    if(operation.callback)
    {
        if(!(reply = myuvamqp_reply_create(MYUVAMQP_BASIC_CONSUME_OPERATION)))
            return -1;
        myuvamqp_channel_reply_init((myuvamqp_reply_t *) reply, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_BASIC_CLASS, MYUVAMQP_BASIC_CONSUME);
        reply->consumer_tag = consumer_tag;
        operation.callback(channel, (myuvamqp_reply_t *) reply, operation.arg);
    }

    return 0;
}


int myuvamqp_channel_basic_cancel(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t consumer_tag,
    myuvamqp_bit_t no_wait,
    void (*cb_basic_cancel)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_octet_t option;
    myuvamqp_operation_t *operation;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if(!(operation = myuvamqp_channel_add_operation(channel, MYUVAMQP_BASIC_CANCEL_OPERATION, cb_basic_cancel, arg)))
        return -1;

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_BASIC_CLASS, MYUVAMQP_BASIC_CANCEL)) < 0)
        goto cleanup;

    option = no_wait;

    if((err = myuvamqp_frame_encode_basic_cancel(&helper, consumer_tag, option)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    MYUVAMQP_REMOVE_OPERATION(channel, operation)
    return err;
}

int myuvamqp_channel_basic_cancel_ok(myuvamqp_channel_t *channel, myuvamqp_short_string_t consumer_tag)
{
    myuvamqp_basic_cancel_reply_t *reply;
    myuvamqp_operation_t operation;

    MYUVAMQP_GET_OPERATION(channel, operation, MYUVAMQP_BASIC_CANCEL_OPERATION);

    if(operation.callback)
    {
        if(!(reply = myuvamqp_reply_create(MYUVAMQP_BASIC_CANCEL_OPERATION)))
            return -1;
        myuvamqp_channel_reply_init((myuvamqp_reply_t *) reply, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_BASIC_CLASS, MYUVAMQP_BASIC_CANCEL);
        reply->consumer_tag = consumer_tag;
        operation.callback(channel, (myuvamqp_reply_t *) reply, operation.arg);
    }

    return 0;
}

int myuvamqp_channel_confirm_select(
    myuvamqp_channel_t *channel,
    myuvamqp_bit_t no_wait,
    void (*cb_confirm_select)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    int err;
    myuvamqp_octet_t option;
    myuvamqp_operation_t *operation;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if(!(operation = myuvamqp_channel_add_operation(channel, MYUVAMQP_CONFIRM_SELECT_OPERATION, cb_confirm_select, arg)))
        return -1;

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_CONFIRM_CLASS, MYUVAMQP_CONFIRM_SELECT)) < 0)
        goto cleanup;

    option = no_wait;

    if((err = myuvamqp_frame_encode_confirm_select(&helper, option)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    MYUVAMQP_REMOVE_OPERATION(channel, operation)
    return err;
}


int myuvamqp_channel_confirm_select_ok(myuvamqp_channel_t *channel)
{
    myuvamqp_confirm_select_reply_t *reply;
    myuvamqp_operation_t operation;

    MYUVAMQP_GET_OPERATION(channel, operation, MYUVAMQP_CONFIRM_SELECT_OPERATION);

    if(operation.callback)
    {
        if(!(reply = myuvamqp_reply_create(MYUVAMQP_CONFIRM_SELECT_OPERATION)))
            return -1;
        myuvamqp_channel_reply_init((myuvamqp_reply_t *) reply, MYUVAMQP_REPLY_CODE_SUCCESS, NULL, MYUVAMQP_CONFIRM_CLASS, MYUVAMQP_CONFIRM_SELECT);
        operation.callback(channel, (myuvamqp_reply_t *) reply, operation.arg);
    }

    return 0;
}

int myuvamqp_channel_basic_publish(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t routing_key,
    myuvamqp_content_t *content,
    myuvamqp_bit_t mandatory,
    myuvamqp_bit_t immediate)
{
    int err;
    myuvamqp_octet_t option = 0;
    myuvamqp_operation_t *operation;
    myuvamqp_buf_helper_t method_frame_helper = {NULL, 0};

    if((err = myuvamqp_frame_encode_header(&method_frame_helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&method_frame_helper, MYUVAMQP_BASIC_CLASS, MYUVAMQP_BASIC_PUBLISH)) < 0)
        goto cleanup;

    option |= mandatory << 0;
    option |= immediate << 1;

    if((err = myuvamqp_frame_encode_basic_publish(&method_frame_helper, exchange, routing_key, option)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &method_frame_helper, NULL, NULL)) < 0)
        goto cleanup;

    if((err = myuvamqp_content_frame_send(content, channel)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(method_frame_helper.base);
    return err;
}

static void myuvamqp_channel_reply_init(
    myuvamqp_reply_t *reply,
    myuvamqp_short_t reply_code,
    myuvamqp_short_string_t reply_text,
    myuvamqp_short_t class_id,
    myuvamqp_short_t method_id)
{
    reply->reply_code = reply_code;
    reply->reply_text = reply_text;
    reply->class_id = class_id;
    reply->method_id = method_id;
}

static myuvamqp_operation_t *myuvamqp_channel_add_operation(
    myuvamqp_channel_t *channel,
    int operation_type,
    void (*callback)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg)
{
    myuvamqp_operation_t *operation = NULL;

    if(channel->operations[operation_type] == NULL)
    {
        channel->operations[operation_type] = myuvamqp_list_create();
        if(!channel->operations[operation_type])
            return NULL;
        myuvamqp_list_set_free_function(channel->operations[operation_type], myuvamqp_free_operation);
    }

    if(!(operation = myuvamqp_mem_alloc(sizeof(*operation))))
        goto cleanup;

    operation->operation_type = operation_type;
    operation->callback = callback;
    operation->arg = arg;

    if(!(myuvamqp_list_insert_tail(channel->operations[operation_type], operation)))
        goto cleanup;

    return operation;

cleanup:
    myuvamqp_list_destroy(channel->operations[operation_type]);
    myuvamqp_mem_free(operation);
    return NULL;
}

static void myuvamqp_free_operation(void *operation)
{
    myuvamqp_mem_free(operation);
}

static void *myuvamqp_reply_create(int operation_type)
{
    void *reply;
    size_t size = 0;

    switch(operation_type)
    {
        case MYUVAMQP_QUEUE_DECLARE_OPERATION:
            size = sizeof(myuvamqp_queue_declare_reply_t);
            break;

        case MYUVAMQP_QUEUE_DELETE_OPERATION:
            size = sizeof(myuvamqp_queue_delete_reply_t);
            break;

        case MYUVAMQP_QUEUE_BIND_OPERATION:
            size = sizeof(myuvamqp_queue_bind_reply_t);
            break;

        case MYUVAMQP_QUEUE_UNBIND_OPERATION:
            size = sizeof(myuvamqp_queue_unbind_reply_t);
            break;

        case MYUVAMQP_EXCHANGE_DECLARE_OPERATION:
            size = sizeof(myuvamqp_exchange_declare_reply_t);
            break;

        case MYUVAMQP_EXCHANGE_DELETE_OPERATION:
            size = sizeof(myuvamqp_exchange_delete_reply_t);
            break;

        case MYUVAMQP_BASIC_QOS_OPERATION:
            size = sizeof(myuvamqp_basic_qos_reply_t);
            break;

        case MYUVAMQP_BASIC_CONSUME_OPERATION:
            size = sizeof(myuvamqp_basic_consume_reply_t);
            break;

        case MYUVAMQP_BASIC_CANCEL_OPERATION:
            size = sizeof(myuvamqp_basic_cancel_reply_t);
            break;

        case MYUVAMQP_CONFIRM_SELECT_OPERATION:
            size = sizeof(myuvamqp_confirm_select_reply_t);
            break;

        default:
            size = 0;
    }

    if(!size)
        reply = NULL;
    else
    {
        if((reply = myuvamqp_mem_alloc(size)))
            memset(reply, 0, size);
    }

    return reply;
}


static int myuvamqp_get_operation_type(myuvamqp_short_t class_id, myuvamqp_short_t method_id)
{
    int operation_type = -1;

    if(class_id == MYUVAMQP_QUEUE_CLASS && method_id == MYUVAMQP_QUEUE_DECLARE)
        operation_type = MYUVAMQP_QUEUE_DECLARE_OPERATION;

    else if(class_id == MYUVAMQP_QUEUE_CLASS && method_id == MYUVAMQP_QUEUE_DELETE)
        operation_type = MYUVAMQP_QUEUE_DELETE_OPERATION;

    else if(class_id == MYUVAMQP_QUEUE_CLASS && method_id == MYUVAMQP_QUEUE_BIND)
        operation_type = MYUVAMQP_QUEUE_BIND_OPERATION;

    else if(class_id == MYUVAMQP_QUEUE_CLASS && method_id == MYUVAMQP_QUEUE_UNBIND)
        operation_type = MYUVAMQP_QUEUE_UNBIND_OPERATION;

    else if(class_id == MYUVAMQP_EXCHANGE_CLASS && method_id == MYUVAMQP_EXCHANGE_DECLARE)
        operation_type = MYUVAMQP_EXCHANGE_DECLARE_OPERATION;

    else if(class_id == MYUVAMQP_EXCHANGE_CLASS && method_id == MYUVAMQP_EXCHANGE_DELETE)
        operation_type = MYUVAMQP_EXCHANGE_DELETE_OPERATION;

    else if(class_id == MYUVAMQP_BASIC_CLASS && method_id == MYUVAMQP_BASIC_QOS)
        operation_type = MYUVAMQP_BASIC_QOS_OPERATION;

    else if(class_id == MYUVAMQP_BASIC_CLASS && method_id == MYUVAMQP_BASIC_CONSUME)
        operation_type = MYUVAMQP_BASIC_CONSUME_OPERATION;

    else if(class_id == MYUVAMQP_BASIC_CLASS && method_id == MYUVAMQP_BASIC_CANCEL)
        operation_type = MYUVAMQP_BASIC_CANCEL_OPERATION;

    else if(class_id == MYUVAMQP_CONFIRM_CLASS && method_id == MYUVAMQP_CONFIRM_SELECT)
        operation_type = MYUVAMQP_CONFIRM_SELECT_OPERATION;

    return operation_type;
}

static int myuvamqp_channel_handle_reply(
    myuvamqp_channel_t *channel,
    myuvamqp_short_t reply_code,
    myuvamqp_short_string_t reply_text,
    myuvamqp_short_t class_id,
    myuvamqp_short_t method_id)
{
    void *reply;
    myuvamqp_operation_t operation;
    int operation_type = myuvamqp_get_operation_type(class_id, method_id);

    assert(operation_type >= 0);

    MYUVAMQP_GET_OPERATION(channel, operation, operation_type);

    if(operation.callback)
    {
        if(!(reply = myuvamqp_reply_create(operation_type)))
            return -1;
        myuvamqp_channel_reply_init(reply, reply_code, reply_text, class_id, method_id);
        operation.callback(channel, reply, operation.arg);
    }

    return 0;
}
