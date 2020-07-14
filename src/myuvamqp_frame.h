#ifndef _MYUVAMQP_FRAME_H
#define _MYUVAMQP_FRAME_H

#include <string.h>
#include "myuvamqp_types.h"
#include "myuvamqp_buffer.h"
#include "myuvamqp_connection.h"

#define MYUVAMQP_FRAME_END 0xCE

#define MYUVAMQP_FRAME_FIELDS       \
    myuvamqp_octet_t frame_type;    \
    myuvamqp_short_t channel_id;    \
    myuvamqp_long_t frame_size;

#define MYUVAMQP_FRAME_SIZE(helper) \
    ((helper->alloc_size) - (sizeof(myuvamqp_octet_t) + sizeof(myuvamqp_short_t) + sizeof(myuvamqp_long_t)))

#define MYUVAMQP_ENCODE_FRAME_SIZE(frame_size)  \
    memcpy(helper->base + (sizeof(myuvamqp_octet_t) + sizeof(myuvamqp_short_t)), &frame_size, sizeof(myuvamqp_long_t))

typedef struct {
    MYUVAMQP_FRAME_FIELDS
} myuvamqp_frame_t;

int myuvamqp_frame_decode_header(
    char **buf,
    myuvamqp_octet_t *frame_type,
    myuvamqp_short_t *channel_id,
    myuvamqp_long_t *frame_size);

int myuvamqp_frame_encode_header(myuvamqp_buf_helper_t *helper, myuvamqp_octet_t frame_type, myuvamqp_short_t channel_id);
int myuvamqp_frame_encode_method_header(myuvamqp_buf_helper_t *helper, myuvamqp_short_t class_id, myuvamqp_short_t method_id);

int myuvamqp_frame_decode_method_header(char **buf, myuvamqp_short_t *class_id, myuvamqp_short_t *method_id);

int myuvamqp_frame_decode_connection_start(
    char **buf,
    myuvamqp_octet_t *version_major,
    myuvamqp_octet_t *version_minor,
    myuvamqp_field_table_t **server_properties,
    myuvamqp_long_string_t *mechanism,
    myuvamqp_long_string_t *locales);

int myuvamqp_frame_encode_connection_start_ok(
    myuvamqp_buf_helper_t *helper,
    const myuvamqp_field_table_t *client_properties,
    const char *mechanism,
    const myuvamqp_field_table_t *response,
    const char *locale);

int myuvamqp_frame_send(
    myuvamqp_connection_t *amqp_conn,
    myuvamqp_buf_helper_t *helper,
    void (*write_cb)(myuvamqp_connection_t *, void *, int),
    void *arg);


int myuvamqp_frame_decode_connection_tune(
    char **buf,
    myuvamqp_short_t *channel_max,
    myuvamqp_long_t *frame_max,
    myuvamqp_short_t *heartbeat);

int myuvamq_frame_encode_connection_tune_ok(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_t channel_max,
    myuvamqp_long_t frame_max,
    myuvamqp_short_t heartbeat);

int myuvamqp_frame_encode_connection_open(myuvamqp_buf_helper_t *helper, const myuvamqp_short_string_t vhost);

int myuvamqp_frame_encode_channel_open(myuvamqp_buf_helper_t *helper);

int myuvamqp_frame_encode_queue_declare(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t queue,
    myuvamqp_octet_t option,
    myuvamqp_field_table_t *arguments);

int myuvamqp_frame_decode_queue_declare_ok(
    char **bufp,
    myuvamqp_short_string_t *queue,
    myuvamqp_long_t *message_count,
    myuvamqp_long_t *consumer_count);

int myuvamqp_frame_encode_queue_delete(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t queue,
    myuvamqp_octet_t option);

int myuvamqp_frame_decode_queue_delete_ok(char **bufp, myuvamqp_long_t *message_count);

int myuvamqp_frame_encode_exchange_declare(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t exchange_type,
    myuvamqp_octet_t option,
    myuvamqp_field_table_t *arguments);

int myuvamqp_frame_decode_exchange_declare_ok(char **bufp);

int myuvamqp_frame_encode_exchange_delete(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t exchange,
    myuvamqp_octet_t option);

int myuvamqp_frame_decode_exchange_delete_ok(char **bufp);

int myuvamqp_frame_encode_channel_close(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_t reply_code,
    myuvamqp_short_string_t reply_text,
    myuvamqp_short_t class_id,
    myuvamqp_short_t method_id);

int myuvamqp_frame_decode_channel_close(
    char **bufp,
    myuvamqp_short_t *reply_code,
    myuvamqp_short_string_t *reply_text,
    myuvamqp_short_t *class_id,
    myuvamqp_short_t *method_id);

int myuvamqp_frame_decode_channel_close_ok(char **bufp);

int myuvamqp_frame_encode_connection_close(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_t reply_code,
    myuvamqp_short_string_t reply_text,
    myuvamqp_short_t class_id,
    myuvamqp_short_t method_id);

int myuvamqp_frame_decode_connection_close_ok(char **bufp);

int myuvamqp_frame_encode_channel_close_ok(myuvamqp_buf_helper_t *helper);

int myuvamqp_frame_encode_queue_bind(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t queue,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t routing_key,
    myuvamqp_octet_t option,
    myuvamqp_field_table_t *arguments);

int myuvamqp_frame_decode_queue_bind_ok(char **bufp);

int myuvamqp_frame_encode_queue_unbind(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t queue,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t routing_key,
    myuvamqp_field_table_t *arguments);

int myuvamqp_frame_decode_queue_unbind_ok(char **bufp);

int myuvamqp_frame_encode_queue_purge(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t queue,
    myuvamqp_octet_t option);

int myuvamqp_frame_decode_queue_purge_ok(char **bufp, myuvamqp_long_t *message_count);

int myuvamqp_frame_encode_basic_qos(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_long_t prefetch_size,
    myuvamqp_short_t prefetch_count,
    myuvamqp_octet_t option);

int myuvamqp_frame_decode_basic_qos_ok(char **bufp);

int myuvamqp_frame_encode_basic_consume(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t queue,
    myuvamqp_short_string_t consumer_tag,
    myuvamqp_octet_t option,
    myuvamqp_field_table_t *arguments);

int myuvamqp_frame_decode_basic_consume_ok(char **bufp, myuvamqp_short_string_t *consumer_tag);

int myuvamqp_frame_encode_basic_cancel(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t consumer_tag,
    myuvamqp_octet_t option);

int myuvamqp_frame_decode_basic_cancel_ok(char **bufp, myuvamqp_short_string_t *consumer_tag);

int myuvamqp_frame_encode_confirm_select(myuvamqp_buf_helper_t *helper, myuvamqp_octet_t option);

int myuvamqp_frame_decode_confirm_select_ok(char **bufp);

int myuvamqp_frame_encode_basic_publish(
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t routing_key,
    myuvamqp_octet_t option);

#endif
