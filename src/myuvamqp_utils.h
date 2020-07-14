#ifndef _UTIL_H
#define _UTIL_H

#include "myuvamqp_buffer.h"

void myuvamqp_dump_frames(const void *buf, size_t len);
void myuvamqp_dump_field_table(myuvamqp_field_table_t *field_table, int more_tab);


#endif
