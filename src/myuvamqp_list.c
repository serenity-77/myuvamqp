/*A simple doubly linked list implementation*/

#include <stdlib.h>
#include <assert.h>
#include "myuvamqp_list.h"
#include "myuvamqp_mem.h"

myuvamqp_list_t *myuvamqp_list_create()
{
    myuvamqp_list_t *list;

    if((list = malloc(sizeof(*list))) == NULL)
        return NULL;

    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
    list->free = NULL;

    return list;
}

myuvamqp_list_t *myuvamqp_list_insert_head(myuvamqp_list_t *list, void *value)
{
    myuvamqp_list_node_t *node;

    if((node = malloc(sizeof(*node))) == NULL)
        return NULL;

    node->value = value;

    if(list->head == NULL)
    {
        node->next = NULL;
        node->prev = NULL;
        list->head = node;
        list->tail = node;
    }
    else
    {
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }

    list->len++;
    return list;
}

myuvamqp_list_t *myuvamqp_list_insert_tail(myuvamqp_list_t *list, void *value)
{
    myuvamqp_list_node_t *node;

    if((node = malloc(sizeof(*node))) == NULL)
        return NULL;

    node->value = value;
    node->next = NULL;

    if(list->head == NULL)
    {
        list->head = node;
        list->tail = node;
        node->prev = NULL;
    }
    else
    {
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
    }

    list->len++;
    return list;
}


void myuvamqp_list_remove_head(myuvamqp_list_t *list)
{
    myuvamqp_list_node_t *head;

    if(!MYUVAMQP_LIST_EMPTY(list))
    {
        head = list->head;
        list->head = head->next;

        if(list->free)
            list->free(head->value);

        MYUVAMQP_LIST_FREE_NODE(head);

        list->len--;
        if(!list->len)
            list->tail = NULL;
    }
}

void myuvamqp_list_remove_tail(myuvamqp_list_t *list)
{
    myuvamqp_list_node_t *tail;

    if(!MYUVAMQP_LIST_EMPTY(list))
    {
        tail = list->tail;

        if(list->len > 1)
        {
            tail->prev->next = NULL;
            list->tail = tail->prev;
        }

        if(list->free)
            list->free(tail->value);

        MYUVAMQP_LIST_FREE_NODE(tail);

        list->len--;
        if(!list->len)
        {
            list->tail = NULL;
            list->head = NULL;
        }
    }
}

void myuvamqp_list_destroy(myuvamqp_list_t *list)
{
    if(list == NULL)
        return;

    while(!MYUVAMQP_LIST_EMPTY(list))
        myuvamqp_list_remove_head(list);

    myuvamqp_mem_free(list);
}

void myuvamqp_list_remove_by_value(myuvamqp_list_t *list, const void *value)
{
    myuvamqp_list_node_t *node, *lnode = NULL;

    if(list == NULL)
        return;

    MYUVAMQP_LIST_FOREACH(list, node)
    {
        if(node->value == value)
        {
            lnode = node;
            break;
        }
    }

    if(lnode == NULL)
        return;

    if(MYUVAMQP_LIST_LEN(list) == 1)
    {
        myuvamqp_list_remove_head(list);
        return;
    }

    if(lnode != NULL)
    {
        if(list->head == lnode)
        {
            list->head = lnode->next;
            list->head->prev = NULL;
        }

        else if(list->tail == lnode)
        {
            list->tail = lnode->prev;
            list->tail->next = NULL;
        }

        else
        {
            lnode->prev->next = lnode->next;
            lnode->next->prev = lnode->prev;
        }

        if(list->free)
            list->free(lnode->value);

        MYUVAMQP_LIST_FREE_NODE(lnode);
        list->len--;
    }
}
