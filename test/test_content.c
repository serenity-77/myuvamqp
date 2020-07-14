#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uv.h>
#include "myuvamqp.h"
#include "utils.h"


void test_content_init();
void test_content_set_get_property1();
void test_content_set_get_property2();


int main(int argc, char *argv[])
{
    test_content_init();
    test_content_set_get_property1();
    test_content_set_get_property2();

    return 0;
}


void test_content_init()
{
    myuvamqp_content_t content;
    myuvamqp_content_init(&content, "Message-1", sizeof("Message-1"), NULL);

    ASSERT(0 == memcmp("Message-1", content.body, sizeof("Message-1")));
    ASSERT(sizeof("Message-1") == content.body_size);
    ASSERT(0 == content.property_flags);
    ASSERT(MYUVAMQP_PROPERTIES_MAX == sizeof(content.properties) / sizeof(content.properties[0]));
    ASSERT(NULL == content.destroy_cb);
}


void test_content_set_get_property1()
{
    myuvamqp_content_t content;
    myuvamqp_content_init(&content, "Message-1", sizeof("Message-1"), NULL);
    myuvamqp_long_t xxx = 3000;
    int i = 0;
    int property_flags = 0;

    myuvamqp_short_string_t content_type = "content-type-test";
    myuvamqp_short_string_t content_encoding = "utf8";
    myuvamqp_field_table_t *headers = myuvamqp_field_table_create();
    myuvamqp_field_table_add(headers, "foo", MYUVAMQP_TABLE_TYPE_SHORT_STRING, "FOO");
    myuvamqp_field_table_add(headers, "bar", MYUVAMQP_TABLE_TYPE_SHORT_STRING, "BAR");
    myuvamqp_field_table_add(headers, "xxx", MYUVAMQP_TABLE_TYPE_LONG_UINT, &xxx);
    myuvamqp_octet_t delivery_mode = 2;
    myuvamqp_octet_t priority = 7;
    myuvamqp_short_string_t correlation_id = "correlation-id-test";
    myuvamqp_short_string_t reply_to = "reply-to-test";
    myuvamqp_short_string_t expiration = "expiration-test";
    myuvamqp_short_string_t message_id = "message-id-test";
    myuvamqp_long_long_t current_timestamp = time(NULL);
    myuvamqp_long_long_t timestamp = current_timestamp;
    myuvamqp_short_string_t type = "type-test";
    myuvamqp_short_string_t user_id = "user-id-test";
    myuvamqp_short_string_t app_id = "app-id-test";

    myuvamqp_content_set_property(&content, MYUVAMQP_CONTENT_TYPE, content_type);
    myuvamqp_content_set_property(&content, MYUVAMQP_CONTENT_ENCODING, content_encoding);
    myuvamqp_content_set_property(&content, MYUVAMQP_HEADERS, headers);
    myuvamqp_content_set_property(&content, MYUVAMQP_DELIVERY_MODE, &delivery_mode);
    myuvamqp_content_set_property(&content, MYUVAMQP_PRIORITY, &priority);
    myuvamqp_content_set_property(&content, MYUVAMQP_CORRELATION_ID, correlation_id);
    myuvamqp_content_set_property(&content, MYUVAMQP_REPLY_TO, reply_to);
    myuvamqp_content_set_property(&content, MYUVAMQP_EXPIRATION, expiration);
    myuvamqp_content_set_property(&content, MYUVAMQP_MESSAGE_ID, message_id);
    myuvamqp_content_set_property(&content, MYUVAMQP_TIMESTAMP, &timestamp);
    myuvamqp_content_set_property(&content, MYUVAMQP_TYPE, type);
    myuvamqp_content_set_property(&content, MYUVAMQP_USER_ID, user_id);
    myuvamqp_content_set_property(&content, MYUVAMQP_APP_ID, app_id);

    for(i = 0; i < 13; i++)
        property_flags |= (1 << (MYUVAMQP_PROPERTY_FLAG_BITS - i));

    ASSERT(property_flags == content.property_flags);

    ASSERT(0 == memcmp(content_type, myuvamqp_content_get_property(&content, MYUVAMQP_CONTENT_TYPE), sizeof("content-type-test")));
    ASSERT(0 == memcmp(content_encoding, myuvamqp_content_get_property(&content, MYUVAMQP_CONTENT_ENCODING), sizeof("utf8")));

    myuvamqp_field_table_t *content_header = myuvamqp_content_get_property(&content, MYUVAMQP_HEADERS);
    ASSERT(0 == memcmp("FOO", myuvamqp_field_table_get(content_header, "foo")->field_value, sizeof("FOO")));
    ASSERT(0 == memcmp("BAR", myuvamqp_field_table_get(content_header, "bar")->field_value, sizeof("BAR")));
    ASSERT(xxx == *(int *) myuvamqp_field_table_get(content_header, "xxx")->field_value);

    ASSERT(delivery_mode == *(myuvamqp_octet_t *) myuvamqp_content_get_property(&content, MYUVAMQP_DELIVERY_MODE));
    ASSERT(priority == *(myuvamqp_octet_t *) myuvamqp_content_get_property(&content, MYUVAMQP_PRIORITY));
    ASSERT(0 == memcmp(correlation_id, myuvamqp_content_get_property(&content, MYUVAMQP_CORRELATION_ID), sizeof("correlation-id-test")));
    ASSERT(0 == memcmp(reply_to, myuvamqp_content_get_property(&content, MYUVAMQP_REPLY_TO), sizeof("reply-to-test")));
    ASSERT(0 == memcmp(expiration, myuvamqp_content_get_property(&content, MYUVAMQP_EXPIRATION), sizeof("expiration-test")));
    ASSERT(0 == memcmp(message_id, myuvamqp_content_get_property(&content, MYUVAMQP_MESSAGE_ID), sizeof("message-id-test")));
    ASSERT(current_timestamp == *(myuvamqp_long_long_t *) myuvamqp_content_get_property(&content, MYUVAMQP_TIMESTAMP));
    ASSERT(0 == memcmp(type, myuvamqp_content_get_property(&content, MYUVAMQP_TYPE), sizeof("type-test")));
    ASSERT(0 == memcmp(user_id, myuvamqp_content_get_property(&content, MYUVAMQP_USER_ID), sizeof("user-id-test")));
    ASSERT(0 == memcmp(app_id, myuvamqp_content_get_property(&content, MYUVAMQP_APP_ID), sizeof("app-id-test")));

    myuvamqp_free_field_table(headers);
}

void test_content_set_get_property2()
{
    myuvamqp_content_t content;
    myuvamqp_content_init(&content, "Message-1", sizeof("Message-1"), NULL);
    myuvamqp_long_t xxx = 3000;
    int property_flags = 0;

    myuvamqp_short_string_t content_type = "content-type-test";
    myuvamqp_field_table_t *headers = myuvamqp_field_table_create();
    myuvamqp_field_table_add(headers, "foo", MYUVAMQP_TABLE_TYPE_SHORT_STRING, "FOO");
    myuvamqp_field_table_add(headers, "bar", MYUVAMQP_TABLE_TYPE_SHORT_STRING, "BAR");
    myuvamqp_field_table_add(headers, "xxx", MYUVAMQP_TABLE_TYPE_LONG_UINT, &xxx);
    myuvamqp_octet_t delivery_mode = 2;
    myuvamqp_long_long_t current_timestamp = time(NULL);
    myuvamqp_long_long_t timestamp = current_timestamp;
    myuvamqp_short_string_t user_id = "user-id-test";

    myuvamqp_content_set_property(&content, MYUVAMQP_CONTENT_TYPE, content_type);
    myuvamqp_content_set_property(&content, MYUVAMQP_HEADERS, headers);
    myuvamqp_content_set_property(&content, MYUVAMQP_DELIVERY_MODE, &delivery_mode);
    myuvamqp_content_set_property(&content, MYUVAMQP_TIMESTAMP, &timestamp);
    myuvamqp_content_set_property(&content, MYUVAMQP_USER_ID, user_id);

    property_flags |= (1 << (MYUVAMQP_PROPERTY_FLAG_BITS - MYUVAMQP_CONTENT_TYPE));
    property_flags |= (1 << (MYUVAMQP_PROPERTY_FLAG_BITS - MYUVAMQP_HEADERS));
    property_flags |= (1 << (MYUVAMQP_PROPERTY_FLAG_BITS - MYUVAMQP_DELIVERY_MODE));
    property_flags |= (1 << (MYUVAMQP_PROPERTY_FLAG_BITS - MYUVAMQP_TIMESTAMP));
    property_flags |= (1 << (MYUVAMQP_PROPERTY_FLAG_BITS - MYUVAMQP_USER_ID));

    ASSERT(property_flags == content.property_flags);

    ASSERT(0 == memcmp(content_type, myuvamqp_content_get_property(&content, MYUVAMQP_CONTENT_TYPE), sizeof("content-type-test")));

    myuvamqp_field_table_t *content_header = myuvamqp_content_get_property(&content, MYUVAMQP_HEADERS);
    ASSERT(0 == memcmp("FOO", myuvamqp_field_table_get(content_header, "foo")->field_value, sizeof("FOO")));
    ASSERT(0 == memcmp("BAR", myuvamqp_field_table_get(content_header, "bar")->field_value, sizeof("BAR")));
    ASSERT(xxx == *(int *) myuvamqp_field_table_get(content_header, "xxx")->field_value);

    ASSERT(delivery_mode == *(myuvamqp_octet_t *) myuvamqp_content_get_property(&content, MYUVAMQP_DELIVERY_MODE));
    ASSERT(current_timestamp == *(myuvamqp_long_long_t *) myuvamqp_content_get_property(&content, MYUVAMQP_TIMESTAMP));
    ASSERT(0 == memcmp(user_id, myuvamqp_content_get_property(&content, MYUVAMQP_USER_ID), sizeof("user-id-test")));

    ASSERT(NULL == myuvamqp_content_get_property(&content, MYUVAMQP_PRIORITY));
    ASSERT(NULL == myuvamqp_content_get_property(&content, MYUVAMQP_APP_ID));

    myuvamqp_free_field_table(headers);
}
