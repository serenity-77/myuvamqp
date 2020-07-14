#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include "myuvamqp_utils.h"

void myuvamqp_dump_frames(const void *buf, size_t len)
{
    char *base = (char *) buf;
    int i = 0;
    for(i = 0; i < len; i++)
    {
        if(isalpha(base[i]) || isprint(base[i]))
            printf("%c", base[i]);
        else if(base[i] == '\t')
            printf("\\t");
        else  if(base[i] == '\n')
            printf("\\n");
        else if(!isprint(base[i]))
            printf("\\x%.2hhx", base[i]);
    }
    printf("\n");
}


void myuvamqp_dump_field_table(myuvamqp_field_table_t *field_table, int more_tab)
{
    int i = 0, k = 0;
    myuvamqp_list_node_t *node;
    myuvamqp_field_table_entry_t *entry;

    MYUVAMQP_LIST_FOREACH(field_table, node)
    {
        entry = node->value;

        for(k = 0; k < more_tab; k++)
            printf("\t");

        printf("%d. %s: ", i + 1, entry->field_name);

        switch(entry->field_type)
        {
            case MYUVAMQP_TABLE_TYPE_FIELD_TABLE:
                printf("\n");
                myuvamqp_dump_field_table((myuvamqp_field_table_t *) entry->field_value, more_tab + 1);
                break;

            case MYUVAMQP_TABLE_TYPE_LONG_STRING:
            case MYUVAMQP_TABLE_TYPE_SHORT_STRING:
                printf("%s\n", (char *) entry->field_value);
                break;

            case MYUVAMQP_TABLE_TYPE_BOOLEAN:
                printf("%hhu\n", *(uint8_t *) entry->field_value);
                break;
        }
        
        i++;
    }
}
