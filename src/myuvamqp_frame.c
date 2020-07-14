#include <assert.h>
#include <string.h>
#include "myuvamqp_buffer.h"
#include "myuvamqp_frame.h"
#include "myuvamqp_utils.h"
#include "myuvamqp_mem.h"
#include "myuvamqp_connection.h"


#define _READ_LONG_STRING_LEN(buf)  \
    do {    \
        long_string_len = myuvamqp_read_uint32((buf));   \
        read_size += sizeof(long_string_len);   \
    } while(0)


#define _READ_SHORT_STRING_LEN(buf)  \
    do {    \
        short_string_len = myuvamqp_read_uint8((buf));   \
        read_size += sizeof(short_string_len);   \
    } while(0)


int myuvamqp_frame_decode_header(
    char **bufp,
    myuvamqp_octet_t *frame_type,
    myuvamqp_short_t *channel_id,
    myuvamqp_long_t *frame_size)
{
    int read_size = 0;

    *frame_type = myuvamqp_read_uint8(*bufp);
    read_size += sizeof(*frame_type);

    *channel_id = myuvamqp_read_uint16(*bufp + read_size);
    read_size += sizeof(*channel_id);

    *frame_size = myuvamqp_read_uint32(*bufp + read_size);
    read_size += sizeof(*frame_size);

    *bufp += read_size;
    return read_size;
}

int myuvamqp_frame_decode_method_header(char **bufp, myuvamqp_short_t *class_id, myuvamqp_short_t *method_id)
{
    int read_size = 0;

    *class_id = myuvamqp_read_uint16(*bufp);
    read_size += sizeof(*class_id);

    *method_id = myuvamqp_read_uint16(*bufp + read_size);
    read_size += sizeof(*method_id);

    *bufp += read_size;
    return read_size;
}

int myuvamqp_frame_decode_connection_start(
    char **bufp,
    myuvamqp_octet_t *version_major,
    myuvamqp_octet_t *version_minor,
    myuvamqp_field_table_t **server_properties,
    myuvamqp_long_string_t *mechanism,
    myuvamqp_long_string_t *locales)
{
    int read_size;
    myuvamqp_long_t field_table_size;
    myuvamqp_long_t long_string_len;

    *server_properties = NULL;
    *mechanism = NULL;
    *locales = NULL;

    *version_major = myuvamqp_read_uint8(*bufp);
    read_size = sizeof(*version_major);

    *version_minor = myuvamqp_read_uint8(*bufp + read_size);
    read_size += sizeof(*version_minor);

    field_table_size = myuvamqp_read_uint32(*bufp + read_size);
    read_size += sizeof(field_table_size);

    *server_properties = myuvamqp_read_field_table(*bufp + read_size, field_table_size);
    if(!*server_properties)
        return -1;
    read_size += field_table_size;

    _READ_LONG_STRING_LEN(*bufp + read_size);
    *mechanism = myuvamqp_read_string(*bufp + read_size, long_string_len);
    if(!*mechanism) goto cleanup;
    read_size += long_string_len;

    _READ_LONG_STRING_LEN(*bufp + read_size);
    *locales = myuvamqp_read_string(*bufp + read_size, long_string_len);
    if(!*locales) goto cleanup;
    read_size += long_string_len;

    *bufp += read_size;
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));

    return read_size;

cleanup:
    myuvamqp_free_field_table(*server_properties);
    myuvamqp_mem_free(*mechanism);
    myuvamqp_mem_free(*locales);
    return -1;
}

int myuvamqp_frame_encode_header(myuvamqp_buf_helper_t *helper, myuvamqp_octet_t frame_type, myuvamqp_short_t channel_id)
{
    int err;

    if((err = myuvamqp_write_uint8(helper, frame_type)) < 0)
        return err;

    if((err = myuvamqp_write_uint16(helper, channel_id)) < 0)
        return err;

    if((err = myuvamqp_write_uint32(helper, 0)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_encode_method_header(myuvamqp_buf_helper_t *helper, myuvamqp_short_t class_id, myuvamqp_short_t method_id)
{
    assert(helper->alloc_size > 0);

    int err;

    if((err = myuvamqp_write_uint16(helper, class_id)) < 0)
        return err;

    if((err = myuvamqp_write_uint16(helper, method_id)) < 0)
        return err;

    return 0;
}


int myuvamqp_frame_encode_connection_start_ok(
    myuvamqp_buf_helper_t *helper,
    const myuvamqp_field_table_t *client_properties,
    const char *mechanism,
    const myuvamqp_field_table_t *response,
    const char *locale)
{
    int err;

    if((err = myuvamqp_write_field_table(client_properties, helper)) < 0)
        return err;

    if((err = myuvamqp_write_short_string(helper, mechanism)) < 0)
        return err;

    if((err = myuvamqp_write_field_table(response, helper)) < 0)
        return err;

    if((err = myuvamqp_write_short_string(helper, locale)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_send(
    myuvamqp_connection_t *amqp_conn,
    myuvamqp_buf_helper_t *helper,
    void (*write_cb)(myuvamqp_connection_t *, void *, int),
    void *arg)
{
    int err;
    myuvamqp_long_t frame_size;

    frame_size = MYUVAMQP_FRAME_SIZE(helper);
    frame_size = myuvamqp_read_uint32(&frame_size);

    MYUVAMQP_ENCODE_FRAME_SIZE(frame_size);

    if((err = myuvamqp_write_uint8(helper, MYUVAMQP_FRAME_END)) < 0)
        return err;

    #ifdef DEBUG_FRAME
        printf("Client Frame: ");
        myuvamqp_dump_frames(helper->base, helper->alloc_size);
    #endif

    return myuvamqp_connection_write(amqp_conn, helper, write_cb, arg);
}

int myuvamqp_frame_decode_connection_tune(
    char **bufp,
    myuvamqp_short_t *channel_max,
    myuvamqp_long_t *frame_max,
    myuvamqp_short_t *heartbeat)
{
    int read_size;

    *channel_max = myuvamqp_read_uint16(*bufp);
    read_size = sizeof(*channel_max);

    *frame_max = myuvamqp_read_uint32(*bufp + read_size);
    read_size += sizeof(*frame_max);

    *heartbeat= myuvamqp_read_uint16(*bufp + read_size);
    read_size += sizeof(*heartbeat);

    *bufp += read_size;
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));

    return read_size;
}

int myuvamq_frame_encode_connection_tune_ok(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_t channel_max,
    myuvamqp_long_t frame_max,
    myuvamqp_short_t heartbeat)
{
    int err;

    if((err = myuvamqp_write_uint16(helper, channel_max)) < 0)
        return err;

    if((err = myuvamqp_write_uint32(helper, frame_max)) < 0)
        return err;

    if((err = myuvamqp_write_uint16(helper, heartbeat)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_encode_connection_open(myuvamqp_buf_helper_t *helper, const myuvamqp_short_string_t vhost)
{
    int err;

    if((err = myuvamqp_write_short_string(helper, vhost)) < 0)
        return err;

    if((err = myuvamqp_write_short_string(helper, NULL)) < 0) // reserved
        return err;

    if((err = myuvamqp_write_uint8(helper, 0)) < 0) // reserved
        return err;

    return 0;
}

int myuvamqp_frame_encode_channel_open(myuvamqp_buf_helper_t *helper)
{
    int err;

    if((err = myuvamqp_write_short_string(helper, NULL)) < 0) // reserved
        return err;

    return 0;
}

int myuvamqp_frame_encode_queue_declare(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t queue,
    myuvamqp_octet_t option,
    myuvamqp_field_table_t *arguments)
{
    int err;

    if((err = myuvamqp_write_uint16(helper, 0)) < 0) //reserved
        return err;

    if((err = myuvamqp_write_short_string(helper, queue)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(helper, option)) < 0)
        return err;

    if((err = myuvamqp_write_field_table(arguments, helper)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_decode_queue_declare_ok(
    char **bufp,
    myuvamqp_short_string_t *queue,
    myuvamqp_long_t *message_count,
    myuvamqp_long_t *consumer_count)
{
    int read_size = 0;
    myuvamqp_octet_t short_string_len;

    _READ_SHORT_STRING_LEN(*bufp + read_size);
    if(!(*queue = myuvamqp_read_string(*bufp + read_size, short_string_len)))
        return -1;
    read_size += short_string_len;

    *message_count = myuvamqp_read_uint32(*bufp + read_size);
    read_size += sizeof(*message_count);

    *consumer_count = myuvamqp_read_uint32(*bufp + read_size);
    read_size += sizeof(*consumer_count);

    *bufp += read_size;
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));

    return read_size;
}

int myuvamqp_frame_encode_queue_delete(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t queue,
    myuvamqp_octet_t option)
{
    int err;

    if((err = myuvamqp_write_uint16(helper, 0)) < 0) //reserved
        return err;

    if((err = myuvamqp_write_short_string(helper, queue)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(helper, option)) < 0)
        return err;

    return err;
}

int myuvamqp_frame_decode_queue_delete_ok(char **bufp, myuvamqp_long_t *message_count)
{
    int read_size = 0;

    *message_count = myuvamqp_read_uint32(*bufp + read_size);
    read_size += sizeof(*message_count);

    *bufp += read_size;
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));

    return read_size;
}

int myuvamqp_frame_encode_exchange_declare(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t exchange_type,
    myuvamqp_octet_t option,
    myuvamqp_field_table_t *arguments)
{
    int err;

    if((err = myuvamqp_write_uint16(helper, 0)) < 0) //reserved
        return err;

    if((err = myuvamqp_write_short_string(helper, exchange)) < 0)
        return err;

    if((err = myuvamqp_write_short_string(helper, exchange_type)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(helper, option)) < 0)
        return err;

    if((err = myuvamqp_write_field_table(arguments, helper)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_decode_exchange_declare_ok(char **bufp)
{
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));
    return 0;
}

int myuvamqp_frame_encode_exchange_delete(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t exchange,
    myuvamqp_octet_t option)
{
    int err;

    if((err = myuvamqp_write_uint16(helper, 0)) < 0) //reserved
        return err;

    if((err = myuvamqp_write_short_string(helper, exchange)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(helper, option)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_decode_exchange_delete_ok(char **bufp)
{
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));
    return 0;
}

int myuvamqp_frame_encode_channel_close(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_t reply_code,
    myuvamqp_short_string_t reply_text,
    myuvamqp_short_t class_id,
    myuvamqp_short_t method_id)
{
    int err;

    if((err = myuvamqp_write_uint16(helper, reply_code)) < 0) //reserved
        return err;

    if((err = myuvamqp_write_short_string(helper, reply_text)) < 0)
        return err;

    if((err = myuvamqp_write_uint16(helper, class_id)) < 0)
        return err;

    if((err = myuvamqp_write_uint16(helper, method_id)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_decode_channel_close_ok(char **bufp)
{
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));
    return 0;
}

int myuvamqp_frame_encode_connection_close(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_t reply_code,
    myuvamqp_short_string_t reply_text,
    myuvamqp_short_t class_id,
    myuvamqp_short_t method_id)
{
    int err;

    if((err = myuvamqp_write_uint16(helper, reply_code)) < 0) //reserved
        return err;

    if((err = myuvamqp_write_short_string(helper, reply_text)) < 0)
        return err;

    if((err = myuvamqp_write_uint16(helper, class_id)) < 0)
        return err;

    if((err = myuvamqp_write_uint16(helper, method_id)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_decode_connection_close_ok(char **bufp)
{
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));
    return 0;
}

int myuvamqp_frame_decode_channel_close(
    char **bufp,
    myuvamqp_short_t *reply_code,
    myuvamqp_short_string_t *reply_text,
    myuvamqp_short_t *class_id,
    myuvamqp_short_t *method_id)
{
    int read_size = 0;
    myuvamqp_octet_t short_string_len;

    *reply_code = myuvamqp_read_uint16(*bufp + read_size);
    read_size += sizeof(*reply_code);

    _READ_SHORT_STRING_LEN(*bufp + read_size);
    if(!(*reply_text = myuvamqp_read_string(*bufp + read_size, short_string_len)))
        return -1;
    read_size += short_string_len;

    *class_id = myuvamqp_read_uint16(*bufp + read_size);
    read_size += sizeof(*class_id);

    *method_id = myuvamqp_read_uint16(*bufp + read_size);
    read_size += sizeof(*method_id);

    *bufp += read_size;
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));

    return read_size;
}

int myuvamqp_frame_encode_queue_bind(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t queue,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t routing_key,
    myuvamqp_octet_t option,
    myuvamqp_field_table_t *arguments)
{
    int err;

    if((err = myuvamqp_write_uint16(helper, 0)) < 0) //reserved
        return err;

    if((err = myuvamqp_write_short_string(helper, queue)) < 0)
        return err;

    if((err = myuvamqp_write_short_string(helper, exchange)) < 0)
        return err;

    if((err = myuvamqp_write_short_string(helper, routing_key)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(helper, option)) < 0)
        return err;

    if((err = myuvamqp_write_field_table(arguments, helper)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_decode_queue_bind_ok(char **bufp)
{
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));
    return 0;
}

int myuvamqp_frame_encode_queue_unbind(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t queue,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t routing_key,
    myuvamqp_field_table_t *arguments)
{
    int err;

    if((err = myuvamqp_write_uint16(helper, 0)) < 0) //reserved
        return err;

    if((err = myuvamqp_write_short_string(helper, queue)) < 0)
        return err;

    if((err = myuvamqp_write_short_string(helper, exchange)) < 0)
        return err;

    if((err = myuvamqp_write_short_string(helper, routing_key)) < 0)
        return err;

    if((err = myuvamqp_write_field_table(arguments, helper)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_decode_queue_unbind_ok(char **bufp)
{
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));
    return 0;
}

int myuvamqp_frame_encode_queue_purge(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t queue,
    myuvamqp_octet_t option)
{
    int err;

    if((err = myuvamqp_write_uint16(helper, 0)) < 0) //reserved
        return err;

    if((err = myuvamqp_write_short_string(helper, queue)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(helper, option)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_decode_queue_purge_ok(char **bufp, myuvamqp_long_t *message_count)
{
    int read_size = 0;

    *message_count = myuvamqp_read_uint32(*bufp + read_size);
    read_size += sizeof(*message_count);

    *bufp += read_size;
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));

    return read_size;
}

int myuvamqp_frame_encode_basic_qos(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_long_t prefetch_size,
    myuvamqp_short_t prefetch_count,
    myuvamqp_octet_t option)
{
    int err;

    if((err = myuvamqp_write_uint32(helper, prefetch_size)) < 0)
        return err;

    if((err = myuvamqp_write_uint16(helper, prefetch_count)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(helper, option)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_decode_basic_qos_ok(char **bufp)
{
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));
    return 0;
}

int myuvamqp_frame_encode_basic_consume(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t queue,
    myuvamqp_short_string_t consumer_tag,
    myuvamqp_octet_t option,
    myuvamqp_field_table_t *arguments)
{
    int err;

    if((err = myuvamqp_write_uint16(helper, 0)) < 0) //reserved
        return err;

    if((err = myuvamqp_write_short_string(helper, queue)) < 0)
        return err;

    if((err = myuvamqp_write_short_string(helper, consumer_tag)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(helper, option)) < 0)
        return err;

    if((err = myuvamqp_write_field_table(arguments, helper)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_decode_basic_consume_ok(char **bufp, myuvamqp_short_string_t *consumer_tag)
{
    int read_size = 0;
    myuvamqp_octet_t short_string_len;

    _READ_SHORT_STRING_LEN(*bufp + read_size);
    if(!(*consumer_tag = myuvamqp_read_string(*bufp + read_size, short_string_len)))
        return -1;
    read_size += short_string_len;

    *bufp += read_size;
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));

    return read_size;
}


int myuvamqp_frame_encode_basic_cancel(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t consumer_tag,
    myuvamqp_octet_t option)
{
    int err;

    if((err = myuvamqp_write_short_string(helper, consumer_tag)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(helper, option)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_decode_basic_cancel_ok(char **bufp, myuvamqp_short_string_t *consumer_tag)
{
    int read_size = 0;
    myuvamqp_octet_t short_string_len;

    _READ_SHORT_STRING_LEN(*bufp + read_size);
    if(!(*consumer_tag = myuvamqp_read_string(*bufp + read_size, short_string_len)))
        return -1;
    read_size += short_string_len;

    *bufp += read_size;
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));

    return read_size;
}

int myuvamqp_frame_encode_confirm_select(myuvamqp_buf_helper_t *helper, myuvamqp_octet_t option)
{
    int err;

    if((err = myuvamqp_write_uint8(helper, option)) < 0)
        return err;

    return 0;
}

int myuvamqp_frame_decode_confirm_select_ok(char **bufp)
{
    assert(MYUVAMQP_FRAME_END == myuvamqp_read_uint8(*bufp));
    return 0;
}


int myuvamqp_frame_encode_basic_publish(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t routing_key,
    myuvamqp_octet_t option)
{
    int err;

    if((err = myuvamqp_write_uint16(helper, 0)) < 0) //reserved
        return err;

    if((err = myuvamqp_write_short_string(helper, exchange)) < 0)
        return err;

    if((err = myuvamqp_write_short_string(helper, routing_key)) < 0)
        return err;

    if((err = myuvamqp_write_uint8(helper, option)) < 0)
        return err;

    return 0;
}
