#ifndef _MYUV_AMQP_BUFFER_H
#define _MYUV_AMQP_BUFFER_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <uv.h>
#include "myuvamqp_list.h"
#include "myuvamqp_types.h"


typedef struct {
    char *base;
    size_t alloc_size;
} myuvamqp_buf_helper_t;


typedef struct {
    char *field_name;
    unsigned char field_type;
    void *field_value;
} myuvamqp_field_table_entry_t;

typedef struct {
    MYUVAMQP_LIST_FIELDS;
} myuvamqp_field_table_t;


#define MYUVAMQP_TABLE_TYPE_SHORT_SHORT_UINT 'B'
#define MYUVAMQP_TABLE_TYPE_SHORT_UINT       'u'
#define MYUVAMQP_TABLE_TYPE_LONG_UINT        'i'
#define MYUVAMQP_TABLE_TYPE_LONG_LONG_UINT   'l'
#define MYUVAMQP_TABLE_TYPE_SHORT_STRING     's'
#define MYUVAMQP_TABLE_TYPE_LONG_STRING      'S'
#define MYUVAMQP_TABLE_TYPE_FIELD_TABLE      'F'
#define MYUVAMQP_TABLE_TYPE_BOOLEAN          't'


#define DEFINE_READ_UINT_PROTOTYPE(name)                                        \
    name##_t myuvamqp_read_##name(void *buf);


extern int _CHECK_ENDIANNESS;


#define DEFINE_READ_UINT_FUNCTION(name)                                         \
    name##_t myuvamqp_read_##name(void *buf)                                    \
    {                                                                           \
        uint8_t *base = (uint8_t *) buf;                                        \
        size_t len = sizeof(name##_t);                                          \
        int i = 0, p = 0;                                                       \
        name##_t result = 0;                                                    \
        if(*(unsigned char *) &_CHECK_ENDIANNESS == 1)                          \
        {                                                                       \
            for(i = len - 1, p = len - 1; i >= 0; i--)                          \
                result |= (name##_t) base[i] << (8 * (p - i));                  \
        }                                                                       \
        else                                                                    \
            result = *(name##_t *) base;                                        \
        return result;                                                          \
    }


#define DEFINE_WRITE_UINT_PROTOTYPE(name)                                       \
    int myuvamqp_write_##name(myuvamqp_buf_helper_t *helper, name##_t val);


#define DEFINE_WRITE_UINT_FUNCTION(name)                                        \
    int myuvamqp_write_##name(myuvamqp_buf_helper_t *helper, name##_t val)      \
    {                                                                           \
        size_t len = sizeof(name##_t);                                          \
        name##_t temp = 0;                                                      \
        uint8_t *ptr = (uint8_t *) &val;                                        \
        void *tmp;                                                              \
        if(!(tmp = myuvamqp_mem_realloc(helper->base, len + helper->alloc_size)))            \
            return -1;                                                          \
        helper->base = tmp;                                                     \
        int i = 0, p = 0;                                                       \
        if(*(unsigned char *) &_CHECK_ENDIANNESS == 1)                          \
        {                                                                       \
            for(i = len - 1, p = len - 1; i >= 0; i--)                          \
                temp |= (name##_t) ptr[i] << (8 * (p - i));                     \
        }                                                                       \
        else                                                                    \
            temp = val;                                                         \
        memcpy(helper->base + helper->alloc_size, &temp, len);                  \
        helper->alloc_size += len;                                              \
        return 0;                                                               \
    }


#define MYUVAMQP_FIELD_TABLE_LEN    MYUVAMQP_LIST_LEN

DEFINE_READ_UINT_PROTOTYPE(uint8)
DEFINE_READ_UINT_PROTOTYPE(uint16)
DEFINE_READ_UINT_PROTOTYPE(uint32)
DEFINE_READ_UINT_PROTOTYPE(uint64)

DEFINE_WRITE_UINT_PROTOTYPE(uint8)
DEFINE_WRITE_UINT_PROTOTYPE(uint16)
DEFINE_WRITE_UINT_PROTOTYPE(uint32)
DEFINE_WRITE_UINT_PROTOTYPE(uint64)


int myuvamqp_write_string(myuvamqp_buf_helper_t *helper, const char *s, size_t len);
int myuvamqp_write_short_string(myuvamqp_buf_helper_t *helper, const char *s);
int myuvamqp_write_long_string(myuvamqp_buf_helper_t *helper, const char *s);

myuvamqp_field_table_t *myuvamqp_read_field_table(const char *base, uint32_t field_table_size);
char *myuvamqp_read_string(const char *base, size_t len);
void myuvamqp_free_field_table(myuvamqp_field_table_t *field_table);
myuvamqp_field_table_t *myuvamqp_field_table_create();
myuvamqp_field_table_t *myuvamqp_field_table_add(
    myuvamqp_field_table_t *field_table,
    const char *field_name,
    unsigned char field_type,
    const void *field_value);

int myuvamqp_write_field_table(const myuvamqp_field_table_t *field_table, myuvamqp_buf_helper_t *helper);
myuvamqp_field_table_entry_t *myuvamqp_field_table_get(myuvamqp_field_table_t *field_table, const char *field_name);

#endif
