#include <string.h>
#include <assert.h>
#include <uv.h>
#include "myuvamqp_list.h"
#include "myuvamqp_mem.h"
#include "myuvamqp_utils.h"
#include "myuvamqp_types.h"
#include "myuvamqp_frame.h"
#include "myuvamqp_channel.h"
#include "myuvamqp_connection.h"

static const char *protocol_init_string = "AMQP";
static const uint8_t protocol_constant = 0x00;
static const uint8_t protocol_version_major = 0x00;
static const uint8_t protocol_version_minor = 0x09;
static const uint8_t protocol_version_revision = 0x01;


static void myuvamqp_connect_cb(uv_connect_t *connect_req, int status);
static int myuvamqp_send_init_string(myuvamqp_connection_t *amqp_conn);
static void myuvamqp_send_init_string_write_cb(myuvamqp_connection_t *amqp_conn, void *arg, int status);
static void myuvamqp_connection_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
static void myuvamqp_connection_alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void myuvamqp_connection_handle_frame_method(
    myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id, myuvamqp_long_t frame_size);
static void myuvamqp_connection_write_cb(uv_write_t *req, int status);
static void myuvamqp_connection_start_ok_write_cb(myuvamqp_connection_t *amqp_conn, void *arg, int status);
static int myuvamqp_do_connection_start(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_connection_tune(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static void myuvamqp_connection_tune_ok_write_cb(myuvamqp_connection_t *amqp_conn, void *arg, int status);
static int myuvamqp_do_connection_open_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static void myuvamqp_connection_free_channel(void *value);
static void myuvamqp_do_channel_open_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_queue_declare_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_queue_delete_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_exchange_declare_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_exchange_delete_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_channel_close_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static void myuvamqp_channel_close_cb(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg);
static int myuvamqp_do_close_connection(myuvamqp_connection_t *amqp_conn);
static int myuvamqp_do_connection_close_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static void myuvamqp_connection_close_cb(uv_handle_t *handle);
static int myuvamqp_do_channel_close(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_queue_bind_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_queue_unbind_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_queue_purge_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_basic_qos_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_basic_consume_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_basic_cancel_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);
static int myuvamqp_do_confirm_select_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id);


struct connection_start_ok_arg {
    myuvamqp_field_table_t *server_properties;
    myuvamqp_long_string_t locales;
    myuvamqp_long_string_t mechanism;
    myuvamqp_field_table_t *response;
};


int myuvamqp_connection_init(uv_loop_t *loop, myuvamqp_connection_t *amqp_conn)
{
    int err;

    err = uv_tcp_init(loop, (uv_tcp_t *) amqp_conn);

    if(!err)
    {
        amqp_conn->user = NULL;
        amqp_conn->password = NULL;
        amqp_conn->vhost = NULL;
        amqp_conn->channels = myuvamqp_list_create();
        myuvamqp_list_set_free_function(amqp_conn->channels, myuvamqp_connection_free_channel);
        amqp_conn->connect_cb.callback = NULL;
        amqp_conn->connect_cb.arg = NULL;
        amqp_conn->auth_cb.callback = NULL;
        amqp_conn->auth_cb.arg = NULL;
        amqp_conn->close_cb.callback = NULL;
        amqp_conn->close_cb.arg = NULL;
        amqp_conn->channel_ids = 0;
        amqp_conn->connection_state = MYUVAMQP_STATE_NOT_CONNECTED;
    }

    return err;
}

int myuvamqp_connection_start(
    myuvamqp_connection_t *amqp_conn,
    const char *host,
    unsigned short port,
    void (*connect_cb)(myuvamqp_connection_t *, void *, int),
    void *arg)
{
    int err;
    uv_connect_t *req;
    struct sockaddr_in hostaddr;

    if(!(req = myuvamqp_mem_alloc(sizeof(*req))))
        return -1;

    memset(&hostaddr, 0, sizeof(hostaddr));
    uv_ip4_addr(host, port, &hostaddr);

    if((err = uv_tcp_connect(req, (uv_tcp_t *) amqp_conn, (struct sockaddr *) &hostaddr, myuvamqp_connect_cb)) < 0)
    {
        myuvamqp_mem_free(req);
        return err;
    }

    amqp_conn->connect_cb.callback = connect_cb;
    amqp_conn->connect_cb.arg = arg;
    amqp_conn->connection_state = MYUVAMQP_STATE_CONNECTING;

    return err;
}

int myuvamqp_connection_write(
    myuvamqp_connection_t *amqp_conn,
    myuvamqp_buf_helper_t *helper,
    void (*write_cb)(myuvamqp_connection_t *, void *, int),
    void *arg)
{
    int err;
    myuvamqp_write_req_t *write_req;
    uv_buf_t buf;

    if(!(write_req = myuvamqp_mem_alloc(sizeof(*write_req))))
        return -1;

    buf = uv_buf_init(helper->base, helper->alloc_size);
    write_req->buf = buf;
    write_req->write_callback = write_cb;
    write_req->arg = arg;

    err = uv_write((uv_write_t *) write_req, (uv_stream_t *) amqp_conn, &buf, 1, myuvamqp_connection_write_cb);
    if(err < 0)
    {
        myuvamqp_mem_free(helper->base);
        myuvamqp_mem_free(write_req);
        return err;
    }

    return err;
}

int myuvamqp_connection_authenticate(
    myuvamqp_connection_t *amqp_conn,
    myuvamqp_long_string_t user,
    myuvamqp_long_string_t password,
    myuvamqp_short_string_t vhost,
    void (*auth_cb)(myuvamqp_connection_t *, void *),
    void *arg)
{
    amqp_conn->user = user;
    amqp_conn->password = password;
    amqp_conn->vhost = vhost;
    amqp_conn->auth_cb.callback = auth_cb;
    amqp_conn->auth_cb.arg = arg;
    // TODO: handle write error
    return myuvamqp_send_init_string(amqp_conn);
}

myuvamqp_channel_t *myuvamqp_connection_create_channel(myuvamqp_connection_t *amqp_conn)
{
    myuvamqp_channel_t *channel;

    if(!(channel = myuvamqp_mem_alloc(sizeof(*channel))))
        return NULL;

    if(myuvamqp_channel_init(channel, amqp_conn, amqp_conn->channel_ids) < 0)
        goto free_channel;

    if(!myuvamqp_list_insert_tail(amqp_conn->channels, channel))
        goto free_channel;

    amqp_conn->channel_ids++;

    return channel;

free_channel:
    myuvamqp_mem_free(channel);
    return NULL;
}

myuvamqp_channel_t *myuvamqp_connection_find_channel_by_id(myuvamqp_connection_t *amqp_conn, myuvamqp_short_t channel_id)
{
    myuvamqp_channel_t *channel_rv = NULL, *channel;
    myuvamqp_list_node_t *node;

    MYUVAMQP_LIST_FOREACH(amqp_conn->channels, node)
    {
        channel = node->value;
        if(channel->channel_id == channel_id)
        {
            channel_rv = channel;
            break;
        }
    }

    return channel_rv;
}

void myuvamqp_connection_set_close_callback(
    myuvamqp_connection_t *amqp_conn,
    void (*close_cb)(myuvamqp_connection_t *, void *),
    void *arg)
{
    amqp_conn->close_cb.callback = close_cb;
    amqp_conn->close_cb.arg = arg;
}

void myuvamqp_connection_close(myuvamqp_connection_t *amqp_conn)
{
    int err;
    myuvamqp_list_node_t *node;
    myuvamqp_channel_t *channel;

    amqp_conn->connection_state = MYUVAMQP_STATE_CLOSING;

    if(MYUVAMQP_LIST_LEN(amqp_conn->channels) > 1)
    {
        MYUVAMQP_LIST_FOREACH(amqp_conn->channels, node)
        {
            channel = node->value;
            if(channel->channel_id)
            {
                err = myuvamqp_channel_close(channel, 0, NULL, 0, 0, myuvamqp_channel_close_cb, amqp_conn);
                assert(0 == err);
            }
        }
    }
    else
    {
        err = myuvamqp_do_close_connection(amqp_conn);
        assert(0 == err);
    }
}

void myuvamqp_connection_remove_channel(myuvamqp_connection_t *amqp_conn, myuvamqp_channel_t *channel)
{
    if(channel->channel_closed)
    {
        myuvamqp_list_remove_by_value(amqp_conn->channels, channel);
    }
}

static void myuvamqp_channel_close_cb(myuvamqp_channel_t *channel, myuvamqp_reply_t *reply, void *arg)
{
    int err;
    myuvamqp_connection_t *connection = arg;
    myuvamqp_channel_t *channel0;

    myuvamqp_mem_free(reply);

    myuvamqp_connection_remove_channel(connection, channel);

    if(MYUVAMQP_LIST_LEN(connection->channels) == 1)
    {
        err = myuvamqp_do_close_connection(connection);
        assert(0 == err);
    }
}

static int myuvamqp_do_close_connection(myuvamqp_connection_t *amqp_conn)
{
    int err;
    myuvamqp_buf_helper_t helper = {NULL, 0};
    myuvamqp_channel_t *channel0 = MYUVAMQP_LIST_HEAD_VALUE(amqp_conn->channels);

    assert(0 == channel0->channel_id);

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel0->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_CONNECTION_CLASS, MYUVAMQP_CONNECTION_CLOSE)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_connection_close(&helper, 0, NULL, 0, 0)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(amqp_conn, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(helper.base);
    return err;
}

static void myuvamqp_connection_free_channel(void *value)
{
    myuvamqp_channel_t *channel = value;
    myuvamqp_channel_destroy(channel);
}


static void myuvamqp_connect_cb(uv_connect_t *connect_req, int status)
{
    myuvamqp_connection_t *amqp_conn = (myuvamqp_connection_t *) connect_req->handle;

    if(!status)
    {
        amqp_conn->connection_state = MYUVAMQP_STATE_CONNECTED;
    }

    if(amqp_conn->connect_cb.callback)
        amqp_conn->connect_cb.callback(amqp_conn, amqp_conn->connect_cb.arg, status);

    myuvamqp_mem_free(connect_req);
}

static void myuvamqp_connection_write_cb(uv_write_t *req, int status)
{
    myuvamqp_write_req_t *write_req = (myuvamqp_write_req_t *) req;
    myuvamqp_connection_t *amqp_conn = (myuvamqp_connection_t *) req->handle;

    if(write_req->write_callback)
        write_req->write_callback(amqp_conn, write_req->arg, status);

    myuvamqp_mem_free(write_req->buf.base);
    myuvamqp_mem_free(write_req);
}

static int myuvamqp_send_init_string(myuvamqp_connection_t *amqp_conn)
{
    int err;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if((err = myuvamqp_write_string(&helper, protocol_init_string, strlen(protocol_init_string))) < 0)
        return err;

    if((err = myuvamqp_write_uint8(&helper, protocol_constant)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(&helper, protocol_version_major)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(&helper, protocol_version_minor)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(&helper, protocol_version_revision)) < 0)
        return err;

    return myuvamqp_connection_write(amqp_conn, &helper, myuvamqp_send_init_string_write_cb, NULL);
}

static void myuvamqp_send_init_string_write_cb(myuvamqp_connection_t *amqp_conn, void *arg, int status)
{
    if(!status)
    {
        uv_read_start((uv_stream_t *) amqp_conn, myuvamqp_connection_alloc_cb, myuvamqp_connection_read_cb);
    }
}

static void myuvamqp_connection_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    myuvamqp_connection_t *amqp_conn = (myuvamqp_connection_t *) stream;
    myuvamqp_octet_t frame_type;
    myuvamqp_short_t channel_id;
    myuvamqp_long_t frame_size, total_size = 0, nsize = 0;
    int read_size;
    char *bufp;

    if(nread > 0)
    {
        bufp = buf->base;

        while(nread != 0)
        {
            read_size = myuvamqp_frame_decode_header(&bufp, &frame_type, &channel_id, &frame_size);

            nsize = read_size + frame_size + 1; // plus 1 FRAME_END

            #ifdef DEBUG_FRAME
                printf("Server Frame: ");
                myuvamqp_dump_frames(buf->base + total_size, nsize);
            #endif

            if(frame_type == MYUVAMQP_FRAME_METHOD)
                myuvamqp_connection_handle_frame_method(amqp_conn, bufp, channel_id, frame_size);

            total_size += nsize;

            bufp += frame_size + 1;
            nread -= nsize;
        }
    }

    myuvamqp_mem_free(buf->base);
}

static void myuvamqp_connection_alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    buf->base = myuvamqp_mem_alloc(suggested_size);
    if(!buf->base) buf->len = 0;
    else buf->len = suggested_size;
}


static void myuvamqp_connection_handle_frame_method(
    myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id, myuvamqp_long_t frame_size)
{
    myuvamqp_short_t class_id, method_id;

    myuvamqp_frame_decode_method_header(&bufp, &class_id, &method_id);

    if(class_id == MYUVAMQP_CONNECTION_CLASS && method_id == MYUVAMQP_CONNECTION_START)
        myuvamqp_do_connection_start(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_CONNECTION_CLASS && method_id == MYUVAMQP_CONNECTION_TUNE)
        myuvamqp_do_connection_tune(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_CONNECTION_CLASS && method_id == MYUVAMQP_CONNECTION_OPEN_OK)
        myuvamqp_do_connection_open_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_CHANNEL_CLASS && method_id == MYUVAMQP_CHANNEL_OPEN_OK)
        myuvamqp_do_channel_open_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_QUEUE_CLASS && method_id == MYUVAMQP_QUEUE_DECLARE_OK)
        myuvamqp_do_queue_declare_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_QUEUE_CLASS && method_id == MYUVAMQP_QUEUE_DELETE_OK)
        myuvamqp_do_queue_delete_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_EXCHANGE_CLASS && method_id == MYUVAMQP_EXCHANGE_DECLARE_OK)
        myuvamqp_do_exchange_declare_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_EXCHANGE_CLASS && method_id == MYUVAMQP_EXCHANGE_DELETE_OK)
        myuvamqp_do_exchange_delete_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_CHANNEL_CLASS && method_id == MYUVAMQP_CHANNEL_CLOSE_OK)
        myuvamqp_do_channel_close_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_CONNECTION_CLASS && method_id == MYUVAMQP_CONNECTION_CLOSE_OK)
        myuvamqp_do_connection_close_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_CHANNEL_CLASS && method_id == MYUVAMQP_CHANNEL_CLOSE)
        myuvamqp_do_channel_close(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_QUEUE_CLASS && method_id == MYUVAMQP_QUEUE_BIND_OK)
        myuvamqp_do_queue_bind_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_QUEUE_CLASS && method_id == MYUVAMQP_QUEUE_UNBIND_OK)
        myuvamqp_do_queue_unbind_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_QUEUE_CLASS && method_id == MYUVAMQP_QUEUE_PURGE_OK)
        myuvamqp_do_queue_purge_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_BASIC_CLASS && method_id == MYUVAMQP_BASIC_QOS_OK)
        myuvamqp_do_basic_qos_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_BASIC_CLASS && method_id == MYUVAMQP_BASIC_CONSUME_OK)
        myuvamqp_do_basic_consume_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_BASIC_CLASS && method_id == MYUVAMQP_BASIC_CANCEL_OK)
        myuvamqp_do_basic_cancel_ok(amqp_conn, bufp, channel_id);

    else if(class_id == MYUVAMQP_CONFIRM_CLASS && method_id == MYUVAMQP_CONFIRM_SELECT_OK)
        myuvamqp_do_confirm_select_ok(amqp_conn, bufp, channel_id);
}


static int myuvamqp_do_connection_start(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int read_size;
    myuvamqp_channel_t *channel;
    myuvamqp_octet_t version_major, version_minor;
    myuvamqp_field_table_t *server_properties, *response = NULL, *client_properties = NULL;
    myuvamqp_long_string_t mechanism;
    myuvamqp_long_string_t locales;
    myuvamqp_buf_helper_t helper = {NULL, 0};
    struct connection_start_ok_arg *csoa;

    if((read_size = myuvamqp_frame_decode_connection_start(
        &bufp, &version_major, &version_minor, &server_properties, &mechanism, &locales)) < 0)
        return read_size;

    if(!(channel = myuvamqp_connection_create_channel(amqp_conn)))
        return -1;

    if(!(response = myuvamqp_field_table_create()))
        goto cleanup;

    if(!myuvamqp_field_table_add(response, "LOGIN", MYUVAMQP_TABLE_TYPE_LONG_STRING, amqp_conn->user))
        goto cleanup;

    if(!myuvamqp_field_table_add(response, "PASSWORD", MYUVAMQP_TABLE_TYPE_LONG_STRING, amqp_conn->password))
        goto cleanup;

    if(myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id) < 0)
        goto cleanup;

    if(myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_CONNECTION_CLASS, MYUVAMQP_CONNECTION_START_OK) < 0)
        goto cleanup;

    if(myuvamqp_frame_encode_connection_start_ok(&helper, client_properties, "AMQPLAIN", response, "en_US") < 0)
        goto cleanup;

    if(!(csoa = myuvamqp_mem_alloc(sizeof(*csoa))))
        goto cleanup;

    csoa->server_properties = server_properties;
    csoa->mechanism = mechanism;
    csoa->locales = locales;
    csoa->response = response;

    if(myuvamqp_frame_send(amqp_conn, &helper, myuvamqp_connection_start_ok_write_cb, csoa) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_free_field_table(server_properties);
    myuvamqp_mem_free(mechanism);
    myuvamqp_mem_free(locales);
    myuvamqp_free_field_table(response);
    myuvamqp_mem_free(helper.base);
    return -1;
}

static void myuvamqp_connection_start_ok_write_cb(myuvamqp_connection_t *amqp_conn, void *arg, int status)
{
    struct connection_start_ok_arg *csoa = arg;

    myuvamqp_free_field_table(csoa->server_properties);
    myuvamqp_mem_free(csoa->mechanism);
    myuvamqp_mem_free(csoa->locales);
    myuvamqp_free_field_table(csoa->response);
    myuvamqp_mem_free(csoa);
}

static int myuvamqp_do_connection_tune(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int read_size, err;
    myuvamqp_channel_t *channel;
    myuvamqp_short_t channel_max;
    myuvamqp_long_t frame_max;
    myuvamqp_short_t heartbeat;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if((read_size = myuvamqp_frame_decode_connection_tune(&bufp, &channel_max, &frame_max, &heartbeat)) < 0)
        return read_size;

    if(!(channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id)))
    {
        err = -1;
        goto cleanup;
    }

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_CONNECTION_CLASS, MYUVAMQP_CONNECTION_TUNE_OK)) < 0)
        goto cleanup;

    // for now, just send what the server send to us
    if((err = myuvamq_frame_encode_connection_tune_ok(&helper, channel_max, frame_max, heartbeat)) < 0)
        goto cleanup;

    return myuvamqp_frame_send(amqp_conn, &helper, myuvamqp_connection_tune_ok_write_cb, channel);

cleanup:
    myuvamqp_mem_free(helper.base);
    return err;
}


static void myuvamqp_connection_tune_ok_write_cb(myuvamqp_connection_t *amqp_conn, void *arg, int status)
{
    int err;
    myuvamqp_channel_t *channel = arg;
    myuvamqp_buf_helper_t helper = {NULL, 0};

    if((err = myuvamqp_frame_encode_header(&helper, MYUVAMQP_FRAME_METHOD, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_method_header(&helper, MYUVAMQP_CONNECTION_CLASS, MYUVAMQP_CONNECTION_OPEN)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_connection_open(&helper, amqp_conn->vhost)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(amqp_conn, &helper, NULL, NULL)) < 0)
        goto cleanup;

    return;

cleanup:
    myuvamqp_mem_free(helper.base);
}


static int myuvamqp_do_connection_open_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    amqp_conn->connection_state = MYUVAMQP_STATE_LOGGEDIN;

    if(amqp_conn->auth_cb.callback)
        amqp_conn->auth_cb.callback(amqp_conn, amqp_conn->auth_cb.arg);

    return 0;
}

static void myuvamqp_do_channel_open_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    myuvamqp_channel_t *channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);

    assert(NULL != channel);

    myuvamqp_channel_open_ok(channel);
}

static int myuvamqp_do_queue_declare_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;
    myuvamqp_short_string_t queue;
    myuvamqp_long_t message_count;
    myuvamqp_long_t consumer_count;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_queue_declare_ok(&bufp, &queue, &message_count, &consumer_count)) < 0)
        return err;

    if((err = myuvamqp_channel_queue_declare_ok(channel, queue, message_count, consumer_count)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(queue);
    return err;
}

static int myuvamqp_do_queue_delete_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;
    myuvamqp_long_t message_count;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_queue_delete_ok(&bufp, &message_count)) < 0)
        return err;

    if((err = myuvamqp_channel_queue_delete_ok(channel, message_count)) < 0)
        return err;

    return 0;
}

static int myuvamqp_do_exchange_declare_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_exchange_declare_ok(&bufp)) < 0)
        return err;

    if((err = myuvamqp_channel_exchange_declare_ok(channel)) < 0)
        return err;

    return 0;
}

static int myuvamqp_do_exchange_delete_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_exchange_delete_ok(&bufp)) < 0)
        return err;

    if((err = myuvamqp_channel_exchange_delete_ok(channel)) < 0)
        return err;

    return 0;
}

static int myuvamqp_do_channel_close_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_channel_close_ok(&bufp)) < 0)
        return err;

    if((err = myuvamqp_channel_close_ok(channel)) < 0)
        return err;

    return 0;
}


static int myuvamqp_do_connection_close_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel0;

    assert(0 == channel_id);

    if((err = myuvamqp_frame_decode_connection_close_ok(&bufp)) < 0)
        return err;

    channel0 = MYUVAMQP_LIST_HEAD_VALUE(amqp_conn->channels);

    channel0->channel_closed = 1;
    channel0->channel_opened = 0;

    myuvamqp_connection_remove_channel(amqp_conn, channel0);

    amqp_conn->connection_state = MYUVAMQP_STATE_NOT_CONNECTED;
    // close the tcp connection
    uv_close((uv_handle_t *) amqp_conn, myuvamqp_connection_close_cb);

    return 0;
}

static void myuvamqp_connection_close_cb(uv_handle_t *handle)
{
    myuvamqp_connection_t *amqp_conn = (myuvamqp_connection_t *) handle;

    myuvamqp_list_destroy(amqp_conn->channels);
    amqp_conn->channels = NULL;
    amqp_conn->channel_ids = 0;

    if(amqp_conn->close_cb.callback)
        amqp_conn->close_cb.callback(amqp_conn, amqp_conn->close_cb.arg);
}

static int myuvamqp_do_channel_close(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;
    myuvamqp_short_t reply_code, class_id, method_id;
    myuvamqp_short_string_t reply_text;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_channel_close(&bufp, &reply_code, &reply_text, &class_id, &method_id)) < 0)
        return err;

    channel->channel_closing = 1;
    if((err = myuvamqp_channel_close(channel, reply_code, reply_text, class_id, method_id, NULL, NULL)) < 0)
        goto cleanup;

    return 0;

cleanup:
    channel->channel_closing = 0;
    myuvamqp_mem_free(reply_text);
    return err;
}

static int myuvamqp_do_queue_bind_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_queue_bind_ok(&bufp)) < 0)
        return err;

    if((err = myuvamqp_channel_queue_bind_ok(channel)) < 0)
        return err;

    return 0;
}

static int myuvamqp_do_queue_unbind_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_queue_unbind_ok(&bufp)) < 0)
        return err;

    if((err = myuvamqp_channel_queue_unbind_ok(channel)) < 0)
        return err;

    return 0;
}

static int myuvamqp_do_queue_purge_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;
    myuvamqp_long_t message_count;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_queue_purge_ok(&bufp, &message_count)) < 0)
        return err;

    if((err = myuvamqp_channel_queue_purge_ok(channel, message_count)) < 0)
        return err;

    return 0;
}

static int myuvamqp_do_basic_qos_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_basic_qos_ok(&bufp)) < 0)
        return err;

    if((err = myuvamqp_channel_basic_qos_ok(channel)) < 0)
        return err;

    return 0;
}

static int myuvamqp_do_basic_consume_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;
    myuvamqp_short_string_t consumer_tag;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_basic_consume_ok(&bufp, &consumer_tag)) < 0)
        return err;

    if((err = myuvamqp_channel_basic_consume_ok(channel, consumer_tag)) < 0)
        return err;

    return 0;
}

static int myuvamqp_do_basic_cancel_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;
    myuvamqp_short_string_t consumer_tag;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_basic_cancel_ok(&bufp, &consumer_tag)) < 0)
        return err;

    if((err = myuvamqp_channel_basic_cancel_ok(channel, consumer_tag)) < 0)
        return err;

    return 0;
}

static int myuvamqp_do_confirm_select_ok(myuvamqp_connection_t *amqp_conn, char *bufp, myuvamqp_short_t channel_id)
{
    int err;
    myuvamqp_channel_t *channel;

    channel = myuvamqp_connection_find_channel_by_id(amqp_conn, channel_id);
    assert(NULL != channel);

    if((err = myuvamqp_frame_decode_confirm_select_ok(&bufp)) < 0)
        return err;

    if((err = myuvamqp_channel_confirm_select_ok(channel)) < 0)
        return err;

    return 0;
}
