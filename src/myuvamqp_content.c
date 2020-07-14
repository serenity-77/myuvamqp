#include <string.h>
#include "myuvamqp_content.h"
#include "myuvamqp_mem.h"
#include "myuvamqp_frame.h"

static void content_write_cb(myuvamqp_connection_t *amqp_conn, void *arg, int status);


int myuvamqp_content_init(
    myuvamqp_content_t *content,
    char *body,
    myuvamqp_long_long_t body_size,
    void (*destroy_cb)(myuvamqp_content_t *))
{
    content->body = body;
    content->body_size = body_size;
    content->property_flags = 0;
    content->destroy_cb = destroy_cb;
    memset(content->properties, 0, sizeof(content->properties));
    return 0;
}


void myuvamqp_content_set_property(
    myuvamqp_content_t *content,
    myuvamqp_property_type_t type,
    void *value)
{
    content->property_flags |= (1 << (MYUVAMQP_PROPERTY_FLAG_BITS - type));
    content->properties[type] = value;
}

void *myuvamqp_content_get_property(myuvamqp_content_t *content, myuvamqp_property_type_t type)
{
    if(!(content->property_flags & (1 << (MYUVAMQP_PROPERTY_FLAG_BITS - type))))
        return NULL;
    return content->properties[type];
}

int myuvamqp_content_encode_content_header_frame(
    myuvamqp_content_t *content,
    myuvamqp_buf_helper_t *helper,
    myuvamqp_short_t class_id)
{
    int err;
    int i;

    if((err = myuvamqp_write_uint16(helper, class_id)) < 0)
        return err;

    if((err = myuvamqp_write_uint16(helper, 0)) < 0)
        return err;

    if((err = myuvamqp_write_uint64(helper, content->body_size)) < 0)
        return err;

    if((err = myuvamqp_write_uint16(helper, content->property_flags)) < 0)
        return err;

    for(i = 0; i < sizeof(content->properties) / sizeof(content->properties[0]); i++)
    {
        if(content->property_flags & (1 << (MYUVAMQP_PROPERTY_FLAG_BITS - i)))
        {
            switch(i)
            {
                case MYUVAMQP_CONTENT_TYPE:
                case MYUVAMQP_CONTENT_ENCODING:
                case MYUVAMQP_CORRELATION_ID:
                case MYUVAMQP_REPLY_TO:
                case MYUVAMQP_EXPIRATION:
                case MYUVAMQP_MESSAGE_ID:
                case MYUVAMQP_TYPE:
                case MYUVAMQP_USER_ID:
                case MYUVAMQP_APP_ID:
                    if((err = myuvamqp_write_short_string(helper, content->properties[i])) < 0) // reserved
                        return err;
                    break;
                case MYUVAMQP_HEADERS:
                    if((err = myuvamqp_write_field_table(content->properties[i], helper)) < 0)
                        return err;
                    break;
                case MYUVAMQP_DELIVERY_MODE:
                case MYUVAMQP_PRIORITY:
                    if((err = myuvamqp_write_uint16(helper, *(myuvamqp_short_t *) content->properties[i])) < 0)
                        return err;
                    break;
                case MYUVAMQP_TIMESTAMP:
                    if((err = myuvamqp_write_uint64(helper, *(myuvamqp_long_long_t *) content->properties[i])) < 0)
                        return err;
                    break;
            }
        }
    }

    return 0;
}

int myuvamqp_content_encode_content_frame(myuvamqp_content_t *content, myuvamqp_buf_helper_t *helper)
{
    void *tmp;
    if(!(tmp = myuvamqp_mem_realloc(helper->base, content->body_size + helper->alloc_size)))
        return -1;
    helper->base = tmp;
    memcpy(helper->base + helper->alloc_size, content->body, content->body_size);
    helper->alloc_size += content->body_size;
    return 0;
}

int myuvamqp_content_frame_send(myuvamqp_content_t *content, myuvamqp_channel_t *channel)
{
    int err;
    myuvamqp_buf_helper_t content_header_helper = {NULL, 0};
    myuvamqp_buf_helper_t content_helper = {NULL, 0};

    if((err = myuvamqp_frame_encode_header(&content_header_helper, MYUVAMQP_FRAME_HEADER, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_encode_header(&content_helper, MYUVAMQP_FRAME_BODY, channel->channel_id)) < 0)
        goto cleanup;

    if((err = myuvamqp_content_encode_content_header_frame(content, &content_header_helper, MYUVAMQP_BASIC_CLASS)) < 0)
        goto cleanup;

    if((err = myuvamqp_content_encode_content_frame(content, &content_helper)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &content_header_helper, NULL, NULL)) < 0)
        goto cleanup;

    if((err = myuvamqp_frame_send(channel->connection, &content_helper, content_write_cb, content)) < 0)
        goto cleanup;

    return 0;

cleanup:
    myuvamqp_mem_free(content_header_helper.base);
    myuvamqp_mem_free(content_helper.base);
    if(content->destroy_cb)
        content->destroy_cb(content);
    return err;
}

static void content_write_cb(myuvamqp_connection_t *amqp_conn, void *arg, int status)
{
    myuvamqp_content_t *content = (myuvamqp_content_t *) arg;
    if(content->destroy_cb)
        content->destroy_cb(content);
}
