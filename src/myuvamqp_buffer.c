#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "myuvamqp_buffer.h"
#include "myuvamqp_mem.h"


int _CHECK_ENDIANNESS = 1;

static void free_field_table_value(void *value);

DEFINE_READ_UINT_FUNCTION(uint8)
DEFINE_READ_UINT_FUNCTION(uint16)
DEFINE_READ_UINT_FUNCTION(uint32)
DEFINE_READ_UINT_FUNCTION(uint64)

DEFINE_WRITE_UINT_FUNCTION(uint8)
DEFINE_WRITE_UINT_FUNCTION(uint16)
DEFINE_WRITE_UINT_FUNCTION(uint32)
DEFINE_WRITE_UINT_FUNCTION(uint64)


int myuvamqp_write_string(myuvamqp_buf_helper_t *helper, const char *s, size_t len)
{
    void *tmp;
    if(!(tmp = myuvamqp_mem_realloc(helper->base, helper->alloc_size + len)))
        return -1;
    helper->base = tmp;
    memcpy(helper->base + helper->alloc_size, s, len);
    helper->alloc_size += len;
    return 0;
}


int myuvamqp_write_short_string(myuvamqp_buf_helper_t *helper, const char *s)
{
    size_t len;
    int err;
    if(s == NULL)
        s = "";
    len = strlen(s);
    if((err = myuvamqp_write_uint8(helper, (uint8_t) len)) < 0)
        return err;
    return myuvamqp_write_string(helper, s, len);
}

int myuvamqp_write_long_string(myuvamqp_buf_helper_t *helper, const char *s)
{
    size_t len;
    int err;
    if(s == NULL)
        s = "";
    len = strlen(s);
    if((err = myuvamqp_write_uint32(helper, (uint32_t) len)) < 0)
        return err;
    return myuvamqp_write_string(helper, s, len);
}

int myuvamqp_write_field_table(const myuvamqp_field_table_t *field_table, myuvamqp_buf_helper_t *helper)
{
    int err = 0;
    size_t alloc_size;
    uint32_t field_table_len;
    myuvamqp_list_node_t *node;
    myuvamqp_field_table_entry_t *entry;

    #define _CHECK_WRITE_ERR(f)    \
        do {    \
            if((err = f) < 0)    \
                return err;   \
        } while(0);

    myuvamqp_write_uint32(helper, 0); // field table length
    alloc_size = helper->alloc_size;

    if(field_table != NULL)
    {
        MYUVAMQP_LIST_FOREACH(field_table, node)
        {
            entry = node->value;
            _CHECK_WRITE_ERR(myuvamqp_write_short_string(helper, entry->field_name));
            _CHECK_WRITE_ERR(myuvamqp_write_string(helper, (char *) &entry->field_type, 1));
            switch(entry->field_type)
            {
                case MYUVAMQP_TABLE_TYPE_SHORT_SHORT_UINT:
                    _CHECK_WRITE_ERR(myuvamqp_write_uint8(helper, *(uint8_t *) entry->field_value));
                    break;
                case MYUVAMQP_TABLE_TYPE_SHORT_UINT:
                    _CHECK_WRITE_ERR(myuvamqp_write_uint16(helper, *(uint16_t *) entry->field_value));
                    break;
                case MYUVAMQP_TABLE_TYPE_LONG_UINT:
                    _CHECK_WRITE_ERR(myuvamqp_write_uint32(helper, *(uint32_t *) entry->field_value));
                    break;
                case MYUVAMQP_TABLE_TYPE_LONG_LONG_UINT:
                    _CHECK_WRITE_ERR(myuvamqp_write_uint64(helper, *(uint64_t *) entry->field_value));
                    break;
                case MYUVAMQP_TABLE_TYPE_SHORT_STRING:
                    _CHECK_WRITE_ERR(myuvamqp_write_short_string(helper, entry->field_value));
                    break;
                case MYUVAMQP_TABLE_TYPE_LONG_STRING:
                    _CHECK_WRITE_ERR(myuvamqp_write_long_string(helper, entry->field_value));
                    break;
                case MYUVAMQP_TABLE_TYPE_FIELD_TABLE:
                    _CHECK_WRITE_ERR(myuvamqp_write_field_table(entry->field_value, helper));
                    break;
            }
        }
    }

    field_table_len = helper->alloc_size - alloc_size;
    field_table_len = myuvamqp_read_uint32(&field_table_len);

    memcpy(helper->base + (alloc_size - sizeof(field_table_len)), &field_table_len, sizeof(field_table_len));
    #undef _CHECK_WRITE_ERR
    return err;
}


myuvamqp_field_table_t *myuvamqp_read_field_table(const char *base, uint32_t field_table_size)
{
    const char *buf = base;
    myuvamqp_field_table_t *field_table;
    size_t len = 0;
    uint8_t field_len;
    char *field_name;
    unsigned char field_type;
    void *field_value;
    uint32_t fts;
    uint8_t uint8_value, short_string_len;
    uint16_t uint16_value;
    uint32_t long_string_len, uint32_value;
    uint64_t uint64_value;
    uint32_t num_read = 0;
    size_t uint_size = 0;

    if(!field_table_size)
        return NULL;

    if(!(field_table = myuvamqp_field_table_create()))
        return NULL;

    while(field_table_size != 0)
    {
        field_len = myuvamqp_read_uint8((void *) buf);
        buf += sizeof(field_len);
        num_read += sizeof(field_len);
        if(!(field_name = myuvamqp_read_string(buf, field_len)))
        {
            field_name = NULL;
            field_value = NULL;
            goto free_field_table;
        }

        buf += field_len;
        num_read += field_len;
        field_type = *buf;
        buf++;
        num_read++;

        switch(field_type)
        {
            case MYUVAMQP_TABLE_TYPE_FIELD_TABLE:
                fts = myuvamqp_read_uint32((char *) buf);
                buf += sizeof(fts);
                num_read += sizeof(fts);
                if(!(field_value = myuvamqp_read_field_table(buf, fts)))
                {
                    field_value = NULL;
                    goto free_field_table;
                }
                buf += fts;
                num_read += fts;
                break;

            case MYUVAMQP_TABLE_TYPE_BOOLEAN:
                uint8_value = myuvamqp_read_uint8((char *) buf);
                uint_size = sizeof(uint8_value);
                buf += uint_size;
                if(!(field_value = myuvamqp_mem_alloc(uint_size)))
                {
                    field_value = NULL;
                    goto free_field_table;
                }
                memcpy(field_value, &uint8_value, uint_size);
                num_read += uint_size;
                break;

            case MYUVAMQP_TABLE_TYPE_SHORT_STRING:
                short_string_len = myuvamqp_read_uint8((char *) buf);
                uint_size = sizeof(short_string_len);
                buf += uint_size;
                num_read += uint_size;
                if(!(field_value = myuvamqp_read_string(buf, (size_t) short_string_len)))
                {
                    if(errno == ENOMEM)
                    {
                        field_value = NULL;
                        goto free_field_table;
                    }
                }
                buf += short_string_len;
                num_read += short_string_len;
                break;

            case MYUVAMQP_TABLE_TYPE_LONG_STRING:
                long_string_len = myuvamqp_read_uint32((char *) buf);
                uint_size = sizeof(long_string_len);
                buf += uint_size;
                num_read += uint_size;
                if(!(field_value = myuvamqp_read_string(buf, (size_t) long_string_len)))
                {
                    if(errno == ENOMEM)
                    {
                        field_value = NULL;
                        goto free_field_table;
                    }
                }
                buf += long_string_len;
                num_read += long_string_len;
                break;

            case MYUVAMQP_TABLE_TYPE_LONG_UINT:
                uint32_value = myuvamqp_read_uint32((char *) buf);
                uint_size = sizeof(uint32_value);
                buf += uint_size;
                if(!(field_value = myuvamqp_mem_alloc(uint_size)))
                {
                    field_value = NULL;
                    goto free_field_table;
                }
                memcpy(field_value, &uint32_value, uint_size);
                num_read += uint_size;
                break;

            case MYUVAMQP_TABLE_TYPE_LONG_LONG_UINT:
                uint64_value = myuvamqp_read_uint64((char *) buf);
                uint_size = sizeof(uint64_value);
                buf += uint_size;
                if(!(field_value = myuvamqp_mem_alloc(uint_size)))
                {
                    field_value = NULL;
                    goto free_field_table;
                }
                memcpy(field_value, &uint64_value, uint_size);
                num_read += uint_size;
                break;

            case MYUVAMQP_TABLE_TYPE_SHORT_SHORT_UINT:
                uint8_value = myuvamqp_read_uint8((char *) buf);
                uint_size = sizeof(uint8_value);
                buf += uint_size;
                if(!(field_value = myuvamqp_mem_alloc(uint_size)))
                {
                    field_value = NULL;
                    goto free_field_table;
                }
                memcpy(field_value, &uint8_value, uint_size);
                num_read += uint_size;
                break;

            case MYUVAMQP_TABLE_TYPE_SHORT_UINT:
                uint16_value = myuvamqp_read_uint16((char *) buf);
                uint_size = sizeof(uint16_value);
                buf += uint_size;
                if(!(field_value = myuvamqp_mem_alloc(uint_size)))
                {
                    field_value = NULL;
                    goto free_field_table;
                }
                memcpy(field_value, &uint16_value, uint_size);
                num_read += uint_size;
                break;
        }

        if(!myuvamqp_field_table_add(field_table, field_name, field_type, field_value))
            goto free_field_table;

        myuvamqp_mem_free(field_name);

        if(field_type != MYUVAMQP_TABLE_TYPE_FIELD_TABLE)
            myuvamqp_mem_free(field_value);

        field_table_size -= num_read;
        num_read = 0;
    }

    return field_table;

free_field_table:
    myuvamqp_mem_free(field_name);
    myuvamqp_mem_free(field_value);
    myuvamqp_free_field_table(field_table);
    return NULL;
}


myuvamqp_field_table_entry_t *myuvamqp_field_table_get(myuvamqp_field_table_t *field_table, const char *field_name)
{
    myuvamqp_field_table_entry_t *entry = NULL;
    myuvamqp_list_node_t *node;

    if(field_table == NULL)
        return NULL;

    MYUVAMQP_LIST_FOREACH(field_table, node)
    {
        entry = node->value;
        if(memcmp(field_name, entry->field_name, strlen(entry->field_name)) == 0)
            return entry;
    }

    return entry;
}

char *myuvamqp_read_string(const char *base, size_t len)
{
    char *buf;

    if(!len)
        return NULL;

    if(!(buf = myuvamqp_mem_alloc(len + 1)))
        return NULL;

    buf[len] = '\0';
    memcpy(buf, base, len);
    return buf;
}


void myuvamqp_free_field_table(myuvamqp_field_table_t *field_table)
{
    if(field_table == NULL)
        return;
    myuvamqp_list_destroy((myuvamqp_list_t *) field_table);
}

static void free_field_table_value(void *value)
{
    myuvamqp_field_table_entry_t *entry = value;

    if(entry == NULL)
        return;

    myuvamqp_mem_free(entry->field_name);

    if(entry->field_type == MYUVAMQP_TABLE_TYPE_FIELD_TABLE)
        myuvamqp_free_field_table(entry->field_value);
    else
        myuvamqp_mem_free(entry->field_value);

    myuvamqp_mem_free(entry);
}

myuvamqp_field_table_t *myuvamqp_field_table_create()
{
    myuvamqp_field_table_t *field_table;

    if(!(field_table = (myuvamqp_field_table_t *) myuvamqp_list_create()))
        return NULL;

    myuvamqp_list_set_free_function(field_table, free_field_table_value);

    return field_table;
}

myuvamqp_field_table_t *myuvamqp_field_table_add(
    myuvamqp_field_table_t *field_table,
    const char *field_name,
    unsigned char field_type,
    const void *field_value)
{
    myuvamqp_field_table_entry_t *entry;
    size_t value_size, field_name_size;
    char *field_value_string;

    if(!(entry = myuvamqp_mem_alloc(sizeof(*entry))))
        return NULL;

    field_name_size = strlen(field_name);
    if(!(entry->field_name = myuvamqp_mem_alloc(field_name_size + 1)))
        goto free_entry;

    entry->field_name[field_name_size] = '\0';
    memcpy(entry->field_name, field_name, field_name_size);

    entry->field_type = field_type;

    switch(field_type)
    {
        case MYUVAMQP_TABLE_TYPE_SHORT_SHORT_UINT:
        case MYUVAMQP_TABLE_TYPE_BOOLEAN:
            value_size = sizeof(uint8_t);
            if(!(entry->field_value = myuvamqp_mem_alloc(value_size)))
                goto free_field_name;
            memcpy(entry->field_value, field_value, value_size);
            break;

        case MYUVAMQP_TABLE_TYPE_SHORT_UINT:
            value_size = sizeof(uint16_t);
            if(!(entry->field_value = myuvamqp_mem_alloc(value_size)))
                goto free_field_name;
            memcpy(entry->field_value, field_value, value_size);
            break;

        case MYUVAMQP_TABLE_TYPE_LONG_UINT:
            value_size = sizeof(uint32_t);
            if(!(entry->field_value = myuvamqp_mem_alloc(value_size)))
                goto free_field_name;
            memcpy(entry->field_value, field_value, value_size);
            break;

        case MYUVAMQP_TABLE_TYPE_LONG_LONG_UINT:
            value_size = sizeof(uint64_t);
            if(!(entry->field_value = myuvamqp_mem_alloc(value_size)))
                goto free_field_name;
            memcpy(entry->field_value, field_value, value_size);
            break;

        case MYUVAMQP_TABLE_TYPE_SHORT_STRING:
        case MYUVAMQP_TABLE_TYPE_LONG_STRING:
            value_size = strlen(field_value);
            if(!(field_value_string = myuvamqp_mem_alloc(value_size + 1)))
                goto free_field_name;
            field_value_string[value_size] = '\0';
            memcpy(field_value_string, field_value, value_size);
            entry->field_value = field_value_string;
            break;

        case MYUVAMQP_TABLE_TYPE_FIELD_TABLE:
            entry->field_value = (void *) field_value;
            break;
    }

    myuvamqp_list_insert_tail((myuvamqp_list_t *) field_table, entry);

    return field_table;

free_entry:
    myuvamqp_mem_free(entry);
    return NULL;

free_field_name:
    myuvamqp_mem_free(entry->field_name);
    myuvamqp_mem_free(entry);
    return NULL;
}
