#include <stdint.h>
#include <string.h>
#include "myuvamqp_buffer.h"
#include "myuvamqp_mem.h"
#include "utils.h"


#define MAKE_TEST_READ_UINT_PROTOTYPE(name)     \
    void test_read_##name();

#define MAKE_TEST_WRITE_UINT_PROTOTYPE(name)    \
    void test_write_##name();


#define MAKE_TEST_READ_UINT_IMPL(name, expected, val)               \
    void test_read_##name()                                         \
    {                                                               \
        name##_t value = val;                                       \
        name##_t result = myuvamqp_read_##name(&value);             \
        ASSERT(expected == result);                                 \
    }


#define MAKE_TEST_WRITE_UINT_IMPL(name, val)                             \
    void test_write_##name()                                             \
    {                                                                    \
        int err;                                                         \
        myuvamqp_buf_helper_t helper = {NULL, 0};                        \
        ASSERT(helper.base == NULL);                                     \
        ASSERT(helper.alloc_size == 0);                                  \
        err = myuvamqp_write_##name(&helper, val);                       \
        ASSERT(helper.alloc_size == sizeof(name##_t));                   \
        name##_t v = val;                                                \
        ASSERT(*(name##_t *) helper.base == myuvamqp_read_##name(&v));   \
        ASSERT(0 == err);                                                \
        myuvamqp_mem_free(helper.base);                                               \
    }


MAKE_TEST_READ_UINT_PROTOTYPE(uint8);
MAKE_TEST_READ_UINT_PROTOTYPE(uint16);
MAKE_TEST_READ_UINT_PROTOTYPE(uint32);
MAKE_TEST_READ_UINT_PROTOTYPE(uint64);
MAKE_TEST_WRITE_UINT_PROTOTYPE(uint8);
MAKE_TEST_WRITE_UINT_PROTOTYPE(uint16);
MAKE_TEST_WRITE_UINT_PROTOTYPE(uint32);
MAKE_TEST_WRITE_UINT_PROTOTYPE(uint64);

void test_write_string();
void test_write_short_string();
void test_write_long_string();
void test_write_field_table();
void test_write_empty_field_table();
void test_read_field_table();

int main(int argc, char *argv[])
{
    test_read_uint8();
    test_read_uint16();
    test_read_uint32();
    test_read_uint64();

    test_write_uint8();
    test_write_uint16();
    test_write_uint32();
    test_write_uint64();
    test_write_string();
    test_write_short_string();
    test_write_long_string();
    test_write_field_table();
    test_write_empty_field_table();
    test_read_field_table();

    return 0;
}

MAKE_TEST_READ_UINT_IMPL(uint8, 0x38, 0x38)
MAKE_TEST_READ_UINT_IMPL(uint16, 0x3412, 0x1234)
MAKE_TEST_READ_UINT_IMPL(uint32, 0x78563412, 0x12345678)
MAKE_TEST_READ_UINT_IMPL(uint64, 0xEFCDAB0078563412, 0x1234567800ABCDEF)
MAKE_TEST_WRITE_UINT_IMPL(uint8, 0x38)
MAKE_TEST_WRITE_UINT_IMPL(uint16, 0x1234)
MAKE_TEST_WRITE_UINT_IMPL(uint32, 0x12345678)
MAKE_TEST_WRITE_UINT_IMPL(uint64, 0x1234567800ABCDEF)


void test_write_string()
{
    int err;
    myuvamqp_buf_helper_t helper = {NULL, 0};
    err = myuvamqp_write_string(&helper, "foobar", strlen("foobar"));
    ASSERT(memcmp(helper.base, "foobar", strlen("foobar")) == 0);
    ASSERT(helper.alloc_size == strlen("foobar"));
    ASSERT(err == 0);
    myuvamqp_mem_free(helper.base);
}

void test_write_short_string()
{
    int err;
    myuvamqp_buf_helper_t helper = {NULL, 0};
    err = myuvamqp_write_short_string(&helper, "foobar");
    ASSERT(memcmp(helper.base + sizeof(uint8_t), "foobar", strlen("foobar")) == 0);
    size_t len = strlen("foobar");
    uint8_t length = myuvamqp_read_uint8(&len);
    ASSERT(memcmp(helper.base, &length, sizeof(uint8_t)) == 0);
    ASSERT(helper.alloc_size == sizeof(uint8_t) + strlen("foobar"));
    ASSERT(0 == err);
    myuvamqp_mem_free(helper.base);
}

void test_write_long_string()
{
    int err;
    myuvamqp_buf_helper_t helper = {NULL, 0};
    err = myuvamqp_write_long_string(&helper, "foobar");
    ASSERT(memcmp(helper.base + sizeof(uint32_t), "foobar", strlen("foobar")) == 0);
    size_t len = strlen("foobar");
    uint32_t length = myuvamqp_read_uint32(&len);
    ASSERT(memcmp(helper.base, &length, sizeof(uint32_t)) == 0);
    ASSERT(helper.alloc_size == sizeof(uint32_t) + strlen("foobar"));
    ASSERT(0 == err);
    myuvamqp_mem_free(helper.base);
}


void test_write_field_table()
{
    myuvamqp_buf_helper_t helper = {NULL, 0};
    int i = 0, err;
    uint32_t expected_length = 0;

    uint8_t val1 = 234;
    uint16_t val2 = 234;
    uint32_t val3 = 234;
    uint64_t val4 = 234;
    char *val5 = "shortstring";
    char *val6 = "longstring";

    myuvamqp_field_table_entry_t field_table_entries[] = {
        {"field1", MYUVAMQP_TABLE_TYPE_SHORT_SHORT_UINT, &val1},
        {"field2", MYUVAMQP_TABLE_TYPE_SHORT_UINT, &val2},
        {"field3", MYUVAMQP_TABLE_TYPE_LONG_UINT, &val3},
        {"field4", MYUVAMQP_TABLE_TYPE_LONG_LONG_UINT, &val4},
        {"field5", MYUVAMQP_TABLE_TYPE_SHORT_STRING, val5},
        {"field6", MYUVAMQP_TABLE_TYPE_LONG_STRING, val6}
    };

    myuvamqp_field_table_t *field_table = myuvamqp_field_table_create();
    myuvamqp_field_table_add(field_table, "field1", MYUVAMQP_TABLE_TYPE_SHORT_SHORT_UINT, &val1);
    myuvamqp_field_table_add(field_table, "field2", MYUVAMQP_TABLE_TYPE_SHORT_UINT, &val2);
    myuvamqp_field_table_add(field_table, "field3", MYUVAMQP_TABLE_TYPE_LONG_UINT, &val3);
    myuvamqp_field_table_add(field_table, "field4", MYUVAMQP_TABLE_TYPE_LONG_LONG_UINT, &val4);
    myuvamqp_field_table_add(field_table, "field5", MYUVAMQP_TABLE_TYPE_SHORT_STRING, val5);
    myuvamqp_field_table_add(field_table, "field6", MYUVAMQP_TABLE_TYPE_LONG_STRING, val6);

    for(i = 0; i < sizeof(field_table_entries) / sizeof(field_table_entries[0]); i++)
    {
        expected_length += sizeof(uint8_t) + strlen(field_table_entries[i].field_name) + 1;
        switch(field_table_entries[i].field_type)
        {
            case MYUVAMQP_TABLE_TYPE_SHORT_SHORT_UINT:
                expected_length += sizeof(uint8_t);
                break;
            case MYUVAMQP_TABLE_TYPE_SHORT_UINT:
                expected_length += sizeof(uint16_t);
                break;
            case MYUVAMQP_TABLE_TYPE_LONG_UINT:
                expected_length += sizeof(uint32_t);
                break;
            case MYUVAMQP_TABLE_TYPE_LONG_LONG_UINT:
                expected_length += sizeof(uint64_t);
                break;
            case MYUVAMQP_TABLE_TYPE_SHORT_STRING:
                expected_length += sizeof(uint8_t);
                expected_length += strlen(field_table_entries[i].field_value);
                break;
            case MYUVAMQP_TABLE_TYPE_LONG_STRING:
                expected_length += sizeof(uint32_t);
                expected_length += strlen(field_table_entries[i].field_value);
                break;
        }
    }

    expected_length = myuvamqp_read_uint32(&expected_length);

    err = myuvamqp_write_field_table(field_table, &helper);

    ASSERT(memcmp(helper.base, &expected_length, sizeof(uint32_t)) == 0);
    ASSERT(helper.alloc_size == (sizeof(uint32_t) + myuvamqp_read_uint32(&expected_length)));
    ASSERT(0 == err);
    myuvamqp_mem_free(helper.base);
    myuvamqp_free_field_table(field_table);
}

void test_write_empty_field_table()
{
    myuvamqp_buf_helper_t helper = {NULL, 0};
    uint32_t length = 0;

    myuvamqp_write_field_table(NULL, &helper);

    ASSERT(memcmp(helper.base, &length, sizeof(uint32_t)) == 0);
    ASSERT(helper.alloc_size == sizeof(uint32_t));
    myuvamqp_mem_free(helper.base);
}

void test_read_field_table()
{
    myuvamqp_buf_helper_t helper = {NULL, 0};

    uint8_t val1 = 234;
    uint16_t val2 = 234;
    uint32_t val3 = 234;
    uint64_t val4 = 234;
    char *val5 = "shortstring";
    char *val6 = "longstring";
    myuvamqp_field_table_t *val7;
    myuvamqp_field_table_t *result;
    uint32_t field_table_len;
    uint16_t ffield2_value = 89;

    val7 = myuvamqp_field_table_create();
    myuvamqp_field_table_add(val7, "ffield1", MYUVAMQP_TABLE_TYPE_LONG_STRING, "foobar");
    myuvamqp_field_table_add(val7, "ffield2", MYUVAMQP_TABLE_TYPE_SHORT_UINT, &ffield2_value);

    myuvamqp_field_table_t *field_table = myuvamqp_field_table_create();
    myuvamqp_field_table_add(field_table, "field1", MYUVAMQP_TABLE_TYPE_SHORT_SHORT_UINT, &val1);
    myuvamqp_field_table_add(field_table, "field2", MYUVAMQP_TABLE_TYPE_SHORT_UINT, &val2);
    myuvamqp_field_table_add(field_table, "field3", MYUVAMQP_TABLE_TYPE_LONG_UINT, &val3);
    myuvamqp_field_table_add(field_table, "field4", MYUVAMQP_TABLE_TYPE_LONG_LONG_UINT, &val4);
    myuvamqp_field_table_add(field_table, "field5", MYUVAMQP_TABLE_TYPE_SHORT_STRING, val5);
    myuvamqp_field_table_add(field_table, "field6", MYUVAMQP_TABLE_TYPE_LONG_STRING, val6);
    myuvamqp_field_table_add(field_table, "field7", MYUVAMQP_TABLE_TYPE_FIELD_TABLE, val7);

    myuvamqp_write_field_table(field_table, &helper);
    myuvamqp_free_field_table(field_table);

    field_table_len = myuvamqp_read_uint32(helper.base);

    result = myuvamqp_read_field_table(helper.base + sizeof(field_table_len), field_table_len);

    ASSERT(0 == memcmp("field1", myuvamqp_field_table_get(result, "field1")->field_name, strlen("field1")));
    ASSERT(MYUVAMQP_TABLE_TYPE_SHORT_SHORT_UINT == myuvamqp_field_table_get(result, "field1")->field_type);
    ASSERT(234 == *(uint8_t *) myuvamqp_field_table_get(result, "field1")->field_value);

    ASSERT(0 == memcmp("field5", myuvamqp_field_table_get(result, "field5")->field_name, strlen("field5")));
    ASSERT(MYUVAMQP_TABLE_TYPE_SHORT_STRING == myuvamqp_field_table_get(result, "field5")->field_type);
    ASSERT(0 == memcmp("shortstring", myuvamqp_field_table_get(result, "field5")->field_value, strlen("shortstring")));

    ASSERT(0 == memcmp("field6", myuvamqp_field_table_get(result, "field6")->field_name, strlen("field6")));
    ASSERT(MYUVAMQP_TABLE_TYPE_LONG_STRING == myuvamqp_field_table_get(result, "field6")->field_type);
    ASSERT(0 == memcmp("longstring", myuvamqp_field_table_get(result, "field6")->field_value, strlen("longstring")));

    ASSERT(0 == memcmp("field7", myuvamqp_field_table_get(result, "field7")->field_name, strlen("field7")));
    ASSERT(MYUVAMQP_TABLE_TYPE_FIELD_TABLE == myuvamqp_field_table_get(result, "field7")->field_type);

    val7 = myuvamqp_field_table_get(result, "field7")->field_value;

    ASSERT(0 == memcmp("ffield1", myuvamqp_field_table_get(val7, "ffield1")->field_name, strlen("ffield1")));
    ASSERT(MYUVAMQP_TABLE_TYPE_LONG_STRING == myuvamqp_field_table_get(val7, "ffield1")->field_type);
    ASSERT(0 == memcmp("foobar", myuvamqp_field_table_get(val7, "ffield1")->field_value, strlen("foobar")));

    ASSERT(0 == memcmp("ffield2", myuvamqp_field_table_get(val7, "ffield2")->field_name, strlen("ffield2")));
    ASSERT(MYUVAMQP_TABLE_TYPE_SHORT_UINT == myuvamqp_field_table_get(val7, "ffield2")->field_type);
    ASSERT(89 == *(uint8_t *) myuvamqp_field_table_get(val7, "ffield2")->field_value);

    myuvamqp_mem_free(helper.base);
    myuvamqp_free_field_table(result);
}
