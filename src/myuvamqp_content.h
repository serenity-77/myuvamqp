#ifndef _MYUVAMQP_CONTENT_H
#define _MYUVAMQP_CONTENT_H

#include "myuvamqp_types.h"
#include "myuvamqp_buffer.h"
#include "myuvamqp_channel.h"


#define MYUVAMQP_PROPERTIES_MAX         13
#define MYUVAMQP_PROPERTY_FLAG_BITS     15

typedef struct myuvamqp_content myuvamqp_content_t;
typedef struct myuvamqp_channel myuvamqp_channel_t;

struct myuvamqp_content {
    char *body;
    myuvamqp_long_long_t body_size;
    myuvamqp_short_t property_flags;
    void *properties[MYUVAMQP_PROPERTIES_MAX];
    void (*destroy_cb)(myuvamqp_content_t *);
};

typedef enum {
    MYUVAMQP_CONTENT_TYPE = 0,
    MYUVAMQP_CONTENT_ENCODING,
    MYUVAMQP_HEADERS,
    MYUVAMQP_DELIVERY_MODE,
    MYUVAMQP_PRIORITY,
    MYUVAMQP_CORRELATION_ID,
    MYUVAMQP_REPLY_TO,
    MYUVAMQP_EXPIRATION,
    MYUVAMQP_MESSAGE_ID,
    MYUVAMQP_TIMESTAMP,
    MYUVAMQP_TYPE,
    MYUVAMQP_USER_ID,
    MYUVAMQP_APP_ID
} myuvamqp_property_type_t;

int myuvamqp_content_init(
    myuvamqp_content_t *content,
    char *body,
    myuvamqp_long_long_t body_size,
    void (*destroy_cb)(myuvamqp_content_t *));


void myuvamqp_content_set_property(
    myuvamqp_content_t *content,
    myuvamqp_property_type_t type,
    void *value);

void *myuvamqp_content_get_property(myuvamqp_content_t *content, myuvamqp_property_type_t type);

int myuvamqp_content_encode_content_header_frame(
    myuvamqp_content_t *content,
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_t class_id);

int myuvamqp_content_encode_content_frame(myuvamqp_content_t *content, myuvamqp_buf_helper_t *helper);

int myuvamqp_content_frame_send(myuvamqp_content_t *content, myuvamqp_channel_t *channel);

#endif
