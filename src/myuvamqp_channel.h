#ifndef _MYUVAMQP_CHANNEL_H
#define _MYUVAMQP_CHANNEL_H

#include "myuvamqp_types.h"
#include "myuvamqp_connection.h"
#include "myuvamqp_content.h"


#define MYUVAMQP_CONNECTION_CLASS                    10
#define MYUVAMQP_CONNECTION_START                    10
#define MYUVAMQP_CONNECTION_START_OK                 11
#define MYUVAMQP_CONNECTION_TUNE                     30
#define MYUVAMQP_CONNECTION_TUNE_OK                  31
#define MYUVAMQP_CONNECTION_OPEN                     40
#define MYUVAMQP_CONNECTION_OPEN_OK                  41
#define MYUVAMQP_CONNECTION_CLOSE                    50
#define MYUVAMQP_CONNECTION_CLOSE_OK                 51
#define MYUVAMQP_CHANNEL_CLASS                       20
#define MYUVAMQP_CHANNEL_OPEN                        10
#define MYUVAMQP_CHANNEL_OPEN_OK                     11
#define MYUVAMQP_CHANNEL_CLOSE                       40
#define MYUVAMQP_CHANNEL_CLOSE_OK                    41
#define MYUVAMQP_QUEUE_CLASS                         50
#define MYUVAMQP_QUEUE_DECLARE                       10
#define MYUVAMQP_QUEUE_DECLARE_OK                    11
#define MYUVAMQP_QUEUE_BIND                          20
#define MYUVAMQP_QUEUE_BIND_OK                       21
#define MYUVAMQP_QUEUE_UNBIND                        50
#define MYUVAMQP_QUEUE_UNBIND_OK                     51
#define MYUVAMQP_QUEUE_PURGE                         30
#define MYUVAMQP_QUEUE_PURGE_OK                      31
#define MYUVAMQP_QUEUE_DELETE                        40
#define MYUVAMQP_QUEUE_DELETE_OK                     41
#define MYUVAMQP_EXCHANGE_CLASS                      40
#define MYUVAMQP_EXCHANGE_DECLARE                    10
#define MYUVAMQP_EXCHANGE_DECLARE_OK                 11
#define MYUVAMQP_EXCHANGE_DELETE                     20
#define MYUVAMQP_EXCHANGE_DELETE_OK                  21
#define MYUVAMQP_BASIC_CLASS                         60
#define MYUVAMQP_BASIC_QOS                           10
#define MYUVAMQP_BASIC_QOS_OK                        11
#define MYUVAMQP_BASIC_CONSUME                       20
#define MYUVAMQP_BASIC_CONSUME_OK                    21
#define MYUVAMQP_BASIC_CANCEL                        30
#define MYUVAMQP_BASIC_CANCEL_OK                     31
#define MYUVAMQP_BASIC_PUBLISH                       40
#define MYUVAMQP_BASIC_DELIVER                       60
#define MYUVAMQP_BASIC_ACK                           80
#define MYUVAMQP_BASIC_REJECT                        90
#define MYUVAMQP_BASIC_NACK                          120
#define MYUVAMQP_CONFIRM_CLASS                       85
#define MYUVAMQP_CONFIRM_SELECT                      10
#define MYUVAMQP_CONFIRM_SELECT_OK                   11


#define MYUVAMQP_REPLY_CODE_SUCCESS                 200
#define MYUVAMQP_REPLY_CODE_CONTENT_TO_LARGE        311
#define MYUVAMQP_REPLY_CODE_NO_ROUTE                312
#define MYUVAMQP_REPLY_CODE_NO_CONSUMERS            311
#define MYUVAMQP_REPLY_CODE_CONNECTION_FORCED       320
#define MYUVAMQP_REPLY_CODE_INVALID_PATH            402
#define MYUVAMQP_REPLY_CODE_ACCESS_REFUSED          403
#define MYUVAMQP_REPLY_CODE_NOT_FOUND               404
#define MYUVAMQP_REPLY_CODE_RESOURCE_LOCKED         405
#define MYUVAMQP_REPLY_CODE_PRECONDITION_FAILED     406
#define MYUVAMQP_REPLY_CODE_FRAME_ERROR             501
#define MYUVAMQP_REPLY_CODE_SYNTAX_ERROR            502
#define MYUVAMQP_REPLY_CODE_COMMAND_INVALID         503
#define MYUVAMQP_REPLY_CODE_CHANNEL_ERROR           504
#define MYUVAMQP_REPLY_CODE_UNEXPECTED_FRAME        505
#define MYUVAMQP_REPLY_CODE_RESOURCE_ERROR          506
#define MYUVAMQP_REPLY_CODE_NOT_ALLOWED             530
#define MYUVAMQP_REPLY_CODE_NOT_IMPLEMENTED         540
#define MYUVAMQP_REPLY_CODE_INTERNAL_ERROR          541


#define MYUVAMQP_QUEUE_DECLARE_OPERATION        0   // done
#define MYUVAMQP_QUEUE_DELETE_OPERATION         1   // done
#define MYUVAMQP_QUEUE_BIND_OPERATION           2   // done
#define MYUVAMQP_QUEUE_UNBIND_OPERATION         3   // done
#define MYUVAMQP_QUEUE_PURGE_OPERATION          4   // done
#define MYUVAMQP_EXCHANGE_DECLARE_OPERATION     5   // done
#define MYUVAMQP_EXCHANGE_DELETE_OPERATION      6   // done
#define MYUVAMQP_EXCHANGE_BIND_OPERATION        7
#define MYUVAMQP_EXCHANGE_UNBIND_OPERATION      8
#define MYUVAMQP_BASIC_QOS_OPERATION            9   // done
#define MYUVAMQP_BASIC_CONSUME_OPERATION        10  // done
#define MYUVAMQP_BASIC_CANCEL_OPERATION         11  // done
#define MYUVAMQP_BASIC_PUBLISH_OPERATION        12  // done
#define MYUVAMQP_BASIC_GET_OPERATION            13
#define MYUVAMQP_TX_SELECT_OPERATION            14
#define MYUVAMQP_TX_COMMIT_OPERATION            15
#define MYUVAMQP_TX_ROLLBACK_OPERATION          16
#define MYUVAMQP_CONFIRM_SELECT_OPERATION       17  // done


typedef struct myuvamqp_connection myuvamqp_connection_t;
typedef struct myuvamqp_channel myuvamqp_channel_t;
typedef struct myuvamqp_reply myuvamqp_reply_t;
typedef struct myuvamqp_content myuvamqp_content_t;

#define MYUVAMQP_OPERATION_CB_FIELDS    \
    void (*callback)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *); \
    void *arg;

typedef struct {
    MYUVAMQP_OPERATION_CB_FIELDS
} myuvamqp_operation_cb_t;

typedef struct {
    int operation_type;
    MYUVAMQP_OPERATION_CB_FIELDS
} myuvamqp_operation_t;

struct myuvamqp_channel {
    myuvamqp_connection_t *connection;
    myuvamqp_short_t channel_id;
    int channel_closed;
    int channel_closing;
    int channel_opened;
    myuvamqp_list_t *operations[20];
    myuvamqp_operation_cb_t cb_open;
    myuvamqp_operation_cb_t cb_close;
};


#define MYUVAMQP_REPLY_FIELDS \
    myuvamqp_short_t reply_code;    \
    myuvamqp_short_string_t reply_text; \
    myuvamqp_short_t class_id;  \
    myuvamqp_short_t method_id;

struct myuvamqp_reply {
    MYUVAMQP_REPLY_FIELDS
};

typedef struct {
    MYUVAMQP_REPLY_FIELDS
    myuvamqp_short_string_t queue;
    myuvamqp_long_t message_count;
    myuvamqp_long_t consumer_count;
} myuvamqp_queue_declare_reply_t;

typedef struct {
    MYUVAMQP_REPLY_FIELDS
    myuvamqp_long_t message_count;
} myuvamqp_queue_delete_reply_t;

typedef struct {
    MYUVAMQP_REPLY_FIELDS
} myuvamqp_exchange_declare_reply_t;

typedef struct {
    MYUVAMQP_REPLY_FIELDS
} myuvamqp_exchange_delete_reply_t;

typedef struct {
    MYUVAMQP_REPLY_FIELDS
} myuvamqp_queue_bind_reply_t;

typedef struct {
    MYUVAMQP_REPLY_FIELDS
} myuvamqp_queue_unbind_reply_t;

typedef struct {
    MYUVAMQP_REPLY_FIELDS
    myuvamqp_long_t message_count;
} myuvamqp_queue_purge_reply_t;

typedef struct {
    MYUVAMQP_REPLY_FIELDS
} myuvamqp_basic_qos_reply_t;

typedef struct {
    MYUVAMQP_REPLY_FIELDS
    myuvamqp_short_string_t consumer_tag;
} myuvamqp_basic_consume_reply_t;

typedef struct {
    MYUVAMQP_REPLY_FIELDS
    myuvamqp_short_string_t consumer_tag;
} myuvamqp_basic_cancel_reply_t;

int myuvamqp_channel_init(myuvamqp_channel_t *channel, myuvamqp_connection_t *amqp_conn, myuvamqp_short_t channel_id);

int myuvamqp_channel_open(
    myuvamqp_channel_t *channel,
    void (*cb_open)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg);

typedef struct {
    MYUVAMQP_REPLY_FIELDS
} myuvamqp_confirm_select_reply_t;

int myuvamqp_channel_open_ok(myuvamqp_channel_t *channel);

void myuvamqp_channel_free_reply(myuvamqp_reply_t *reply);
void myuvamqp_channel_destroy(myuvamqp_channel_t *channel);

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
    void *arg);

int myuvamqp_channel_queue_declare_ok(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t queue,
    myuvamqp_long_t message_count,
    myuvamqp_long_t consumer_count);

int myuvamqp_channel_queue_delete(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t queue,
    myuvamqp_bit_t if_unused,
    myuvamqp_bit_t if_empty,
    myuvamqp_bit_t no_wait,
    void (*queue_delete_cb)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg);

int myuvamqp_channel_queue_delete_ok(myuvamqp_channel_t *channel, myuvamqp_long_t message_count);

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
    void *arg);

int myuvamqp_channel_exchange_declare_ok(myuvamqp_channel_t *channel);

int myuvamqp_channel_exchange_delete(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t exchange,
    myuvamqp_bit_t if_unused,
    myuvamqp_bit_t no_wait,
    void (*exchange_delete_cb)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg);

int myuvamqp_channel_exchange_delete_ok(myuvamqp_channel_t *channel);

int myuvamqp_channel_close(
    myuvamqp_channel_t *channel,
    myuvamqp_short_t reply_code,
    myuvamqp_short_string_t reply_text,
    myuvamqp_short_t class_id,
    myuvamqp_short_t method_id,
    void (*cb_close)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg);

int myuvamqp_channel_close_ok(myuvamqp_channel_t *channel);

int myuvamqp_channel_queue_bind(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t queue,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t routing_key,
    myuvamqp_bit_t no_wait,
    myuvamqp_field_table_t *arguments,
    void (*cb_bind)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg);

int myuvamqp_channel_queue_bind_ok(myuvamqp_channel_t *channel);

int myuvamqp_channel_queue_unbind(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t queue,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t routing_key,
    myuvamqp_field_table_t *arguments,
    void (*cb_bind)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg);

int myuvamqp_channel_queue_unbind_ok(myuvamqp_channel_t *channel);

int myuvamqp_channel_queue_purge(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t queue,
    myuvamqp_bit_t no_wait,
    void (*cb_purge)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg);

int myuvamqp_channel_queue_purge_ok(myuvamqp_channel_t *channel, myuvamqp_long_t message_count);

int myuvamqp_channel_basic_qos(
    myuvamqp_channel_t *channel,
    myuvamqp_long_t prefetch_size,
    myuvamqp_short_t prefetch_count,
    myuvamqp_bit_t global,
    void (*cb_basic_qos)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg);

int myuvamqp_channel_basic_qos_ok(myuvamqp_channel_t *channel);

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
    void *arg);


int myuvamqp_channel_basic_consume_ok(myuvamqp_channel_t *channel, myuvamqp_short_string_t consumer_tag);

int myuvamqp_channel_basic_cancel(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t consumer_tag,
    myuvamqp_bit_t no_wait,
    void (*cb_basic_cancel)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg);

int myuvamqp_channel_basic_cancel_ok(myuvamqp_channel_t *channel, myuvamqp_short_string_t consumer_tag);

int myuvamqp_channel_confirm_select(
    myuvamqp_channel_t *channel,
    myuvamqp_bit_t no_wait,
    void (*cb_confirm_select)(myuvamqp_channel_t *, myuvamqp_reply_t *, void *),
    void *arg);

int myuvamqp_channel_confirm_select_ok(myuvamqp_channel_t *channel);

int myuvamqp_channel_basic_publish(
    myuvamqp_channel_t *channel,
    myuvamqp_short_string_t exchange,
    myuvamqp_short_string_t routing_key,
    myuvamqp_content_t *content,
    myuvamqp_bit_t mandatory,
    myuvamqp_bit_t immediate);

#endif
