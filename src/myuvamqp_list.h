#ifndef _MYUVAMQP_LIST_H
#define _MYUVAMQP_LIST_H

#include <stddef.h>


#define MYUVAMQP_LIST_FOREACH(list, node) \
    for(node = (list)->head; node != NULL; node = node->next)

#define MYUVAMQP_LIST_EMPTY(list) ((list)->head == NULL && (list)->len == 0)

#define MYUVAMQP_LIST_FREE_NODE(node)   \
    do {    \
        (node)->value = NULL;   \
        (node)->prev = NULL;    \
        (node)->next = NULL;    \
        myuvamqp_mem_free(node);    \
    } while(0)

#define MYUVAMQP_LIST_LEN(list)   ((list)->len)

#define MYUVAMQP_LIST_HEAD_VALUE(list)   \
    (list)->head->value

#define myuvamqp_list_set_free_function(list, f) ((list)->free = f)
#define myuvamqp_list_set_find_function(list, f) ((list)->find = f)


typedef struct myuvamqp_list myuvamqp_list_t;
typedef struct myuvamqp_list_node myuvamqp_list_node_t;

#define MYUVAMQP_LIST_FIELDS    \
    myuvamqp_list_node_t *head; \
    myuvamqp_list_node_t *tail; \
    size_t len; \
    void (*free)(void *);   \
    void *(*find)(void *value, void *key);  \


struct myuvamqp_list {
    MYUVAMQP_LIST_FIELDS
};


struct myuvamqp_list_node {
    void *value;
    myuvamqp_list_node_t *prev;
    myuvamqp_list_node_t *next;
};


myuvamqp_list_t *myuvamqp_list_create();
myuvamqp_list_t *myuvamqp_list_insert_head(myuvamqp_list_t *list, void *value);
myuvamqp_list_t *myuvamqp_list_insert_tail(myuvamqp_list_t *list, void *value);
void myuvamqp_list_remove_head(myuvamqp_list_t *list);
void myuvamqp_list_remove_tail(myuvamqp_list_t *list);
void myuvamqp_list_destroy(myuvamqp_list_t *list);
void myuvamqp_list_remove_by_value(myuvamqp_list_t *list, const void *value);

#endif
