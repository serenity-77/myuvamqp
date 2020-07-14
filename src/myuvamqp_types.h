#ifndef _MYUVAMQP_TYPES_H
#define _MYUVAMQP_TYPES_H

#include <stdint.h>

typedef uint8_t myuvamqp_octet_t;
typedef uint16_t myuvamqp_short_t;
typedef uint32_t myuvamqp_long_t;
typedef uint64_t myuvamqp_long_long_t;
typedef char * myuvamqp_short_string_t;
typedef char * myuvamqp_long_string_t;

typedef enum {
    FALSE = 0,
    TRUE
} myuvamqp_bit_t;

#endif
