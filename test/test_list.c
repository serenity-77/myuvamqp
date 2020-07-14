#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <myuvamqp.h>
#include "utils.h"


void test_list_create();
void test_list_insert_head();
void test_list_insert_tail();
void test_list_insert_head_tail();
void test_list_remove_head();
void test_list_remove_tail();
void test_list_destroy();
void test_list_remove_by_value();


int main(int argc, char *argv[])
{
    test_list_create();
    test_list_insert_head();
    test_list_insert_tail();
    test_list_insert_head_tail();
    test_list_remove_head();
    test_list_remove_tail();
    test_list_destroy();
    test_list_remove_by_value();

    return 0;
}


void test_list_create()
{
    myuvamqp_list_t *list = myuvamqp_list_create();

    ASSERT(0 == list->len);
    ASSERT(NULL == list->head);
    ASSERT(NULL == list->tail);

    myuvamqp_mem_free(list);
}

void test_list_insert_head()
{
    int i;
    myuvamqp_list_t *list = myuvamqp_list_create();
    myuvamqp_list_t *rlist;
    myuvamqp_list_node_t *node;
    myuvamqp_list_node_t *nodes[3];

    struct foo {
        int a;
        char *b;
    } value1 = {5, "FooBar"};

    char *value2 = "Value2";
    int value3 = 76;

    rlist = myuvamqp_list_insert_head(list, &value1);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_head(list, value2);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_head(list, &value3);
    ASSERT(rlist == list);

    node = list->head;
    nodes[0] = node;
    ASSERT(&value3 == node->value);
    ASSERT(76 == *(int *) node->value);

    node = node->next;
    nodes[1] = node;
    ASSERT(value2 == node->value);
    ASSERT(0 == memcmp(value2, (char *) node->value, strlen(value2)));

    node = node->next;
    nodes[2] = node;
    ASSERT(&value1 == node->value);
    ASSERT(value1.a == ((struct foo *) node->value)->a);
    ASSERT(0 == memcmp(value1.b, ((struct foo *) node->value)->b, strlen(value1.b)));

    ASSERT(3 == list->len);
    ASSERT(NULL == node->next);
    ASSERT(nodes[2] == list->tail);

    for(i = 0; i < list->len; i++)
    {
        myuvamqp_mem_free(nodes[i]);
    }

    myuvamqp_mem_free(list);
}

void test_list_insert_tail()
{
    int i;
    myuvamqp_list_t *list = myuvamqp_list_create();
    myuvamqp_list_t *rlist;
    myuvamqp_list_node_t *node;
    myuvamqp_list_node_t *nodes[3];

    struct foo {
        int a;
        char *b;
    } value1 = {5, "FooBar"};

    char *value2 = "Value2";
    int value3 = 76;

    rlist = myuvamqp_list_insert_tail(list, &value1);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, value2);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, &value3);
    ASSERT(rlist == list);

    node = list->head;
    nodes[0] = node;
    ASSERT(&value1 == node->value);
    ASSERT(value1.a == ((struct foo *) node->value)->a);
    ASSERT(0 == memcmp(value1.b, ((struct foo *) node->value)->b, strlen(value1.b)));

    node = node->next;
    nodes[1] = node;
    ASSERT(value2 == node->value);
    ASSERT(0 == memcmp(value2, (char *) node->value, strlen(value2)));

    node = node->next;
    nodes[2] = node;
    ASSERT(&value3 == node->value);
    ASSERT(76 == *(int *) node->value);

    ASSERT(3 == list->len);
    ASSERT(NULL == node->next);
    ASSERT(nodes[2] == list->tail);

    for(i = 0; i < list->len; i++)
    {
        myuvamqp_mem_free(nodes[i]);
    }

    myuvamqp_mem_free(list);
}


void test_list_insert_head_tail()
{
    int i;
    myuvamqp_list_t *list = myuvamqp_list_create();
    myuvamqp_list_t *rlist;
    myuvamqp_list_node_t *node;
    myuvamqp_list_node_t *nodes[6];

    struct foo {
        int a;
        char *b;
    } value1 = {5, "FooBar"};

    char *value2 = "Value2";
    int value3 = 76;
    int value4 = 44;
    int value5 = 55;
    int value6 = 98;

    rlist = myuvamqp_list_insert_head(list, &value1);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_head(list, value2);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_head(list, &value3);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, &value4);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, &value5);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_head(list, &value6);
    ASSERT(rlist == list);

    node = list->head;
    nodes[0] = node;
    ASSERT(&value6 == node->value);
    ASSERT(98 == *(int *) node->value);

    node = node->next;
    nodes[1] = node;
    ASSERT(&value3 == node->value);
    ASSERT(76 == *(int *) node->value);

    node = node->next;
    nodes[2] = node;
    ASSERT(value2 == node->value);
    ASSERT(0 == memcmp(value2, (char *) node->value, strlen(value2)));

    node = node->next;
    nodes[3] = node;
    ASSERT(&value1 == node->value);
    ASSERT(value1.a == ((struct foo *) node->value)->a);
    ASSERT(0 == memcmp(value1.b, ((struct foo *) node->value)->b, strlen(value1.b)));

    node = node->next;
    nodes[4] = node;
    ASSERT(&value4 == node->value);
    ASSERT(44 == *(int *) node->value);

    node = node->next;
    nodes[5] = node;
    ASSERT(&value5 == node->value);
    ASSERT(55 == *(int *) node->value);

    ASSERT(6 == list->len);
    ASSERT(NULL == node->next);
    ASSERT(nodes[5] == list->tail);

    for(i = 0; i < list->len; i++)
    {
        myuvamqp_mem_free(nodes[i]);
    }

    myuvamqp_mem_free(list);
}

void test_list_remove_head()
{
    myuvamqp_list_t *list = myuvamqp_list_create();
    myuvamqp_list_t *rlist;
    myuvamqp_list_node_t *node;

    struct foo {
        int a;
        char *b;
    } value1 = {5, "FooBar"};

    char *value2 = "Value2";
    int value3 = 76;

    rlist = myuvamqp_list_insert_tail(list, &value1);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, value2);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, &value3);
    ASSERT(rlist == list);

    node = list->head;
    ASSERT(&value1 == node->value);
    ASSERT(value1.a == ((struct foo *) node->value)->a);
    ASSERT(0 == memcmp(value1.b, ((struct foo *) node->value)->b, strlen(value1.b)));

    myuvamqp_list_remove_head(list);

    node = list->head;
    ASSERT(value2 == node->value);
    ASSERT(0 == memcmp(value2, (char *) node->value, strlen(value2)));

    myuvamqp_list_remove_head(list);

    node = list->head;
    ASSERT(&value3 == node->value);
    ASSERT(76 == *(int *) node->value);

    myuvamqp_list_remove_head(list);

    ASSERT(1 == MYUVAMQP_LIST_EMPTY(list));
    ASSERT(NULL == list->head);
    ASSERT(NULL == list->tail);
    ASSERT(0 == list->len);

    myuvamqp_mem_free(list);
}


void test_list_remove_tail()
{
    myuvamqp_list_t *list = myuvamqp_list_create();
    myuvamqp_list_t *rlist;
    myuvamqp_list_node_t *node;

    struct foo {
        int a;
        char *b;
    } value1 = {5, "FooBar"};

    char *value2 = "Value2";
    int value3 = 76;

    rlist = myuvamqp_list_insert_head(list, &value1);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_head(list, value2);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_head(list, &value3);
    ASSERT(rlist == list);

    node = list->tail;
    ASSERT(&value1 == node->value);
    ASSERT(value1.a == ((struct foo *) node->value)->a);
    ASSERT(0 == memcmp(value1.b, ((struct foo *) node->value)->b, strlen(value1.b)));

    myuvamqp_list_remove_tail(list);

    node = list->tail;
    ASSERT(value2 == node->value);
    ASSERT(0 == memcmp(value2, (char *) node->value, strlen(value2)));

    myuvamqp_list_remove_tail(list);

    node = list->tail;
    ASSERT(&value3 == node->value);
    ASSERT(76 == *(int *) node->value);

    myuvamqp_list_remove_tail(list);

    ASSERT(1 == MYUVAMQP_LIST_EMPTY(list));
    ASSERT(NULL == list->head);
    ASSERT(NULL == list->tail);
    ASSERT(0 == list->len);

    myuvamqp_mem_free(list);
}


void test_list_destroy()
{
    myuvamqp_list_t *list = myuvamqp_list_create();
    myuvamqp_list_t *rlist;
    myuvamqp_list_node_t *node;
    myuvamqp_list_node_t *nodes[6];

    struct foo {
        int a;
        char *b;
    } value1 = {5, "FooBar"};

    char *value2 = "Value2";
    int value3 = 76;
    int value4 = 44;
    int value5 = 55;
    int value6 = 98;

    rlist = myuvamqp_list_insert_head(list, &value1);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_head(list, value2);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_head(list, &value3);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, &value4);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, &value5);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_head(list, &value6);
    ASSERT(rlist == list);

    ASSERT(6 == MYUVAMQP_LIST_LEN(list));
    ASSERT(6 == MYUVAMQP_LIST_LEN(rlist));

    node = list->head;
    nodes[0] = node;
    ASSERT(&value6 == node->value);
    ASSERT(98 == *(int *) node->value);

    node = node->next;
    nodes[1] = node;
    ASSERT(&value3 == node->value);
    ASSERT(76 == *(int *) node->value);

    node = node->next;
    nodes[2] = node;
    ASSERT(value2 == node->value);
    ASSERT(0 == memcmp(value2, (char *) node->value, strlen(value2)));

    node = node->next;
    nodes[3] = node;
    ASSERT(&value1 == node->value);
    ASSERT(value1.a == ((struct foo *) node->value)->a);
    ASSERT(0 == memcmp(value1.b, ((struct foo *) node->value)->b, strlen(value1.b)));

    node = node->next;
    nodes[4] = node;
    ASSERT(&value4 == node->value);
    ASSERT(44 == *(int *) node->value);

    node = node->next;
    nodes[5] = node;
    ASSERT(&value5 == node->value);
    ASSERT(55 == *(int *) node->value);

    ASSERT(6 == list->len);
    ASSERT(NULL == node->next);
    ASSERT(nodes[5] == list->tail);

    myuvamqp_list_destroy(list);
}

void test_list_remove_by_value()
{
    int i = 0;
    myuvamqp_list_t *list = myuvamqp_list_create();
    myuvamqp_list_node_t *node;
    myuvamqp_list_t *rlist;

    char *value1 = "Value1";
    char *value2 = "Value2";
    int value3 = 76;
    int value4 = 44;
    int value5 = 55;
    int value6 = 98;

    rlist = myuvamqp_list_insert_tail(list, value1);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, value2);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, &value3);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, &value4);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, &value5);
    ASSERT(rlist == list);
    rlist = myuvamqp_list_insert_tail(list, &value6);
    ASSERT(rlist == list);

    ASSERT(6 == MYUVAMQP_LIST_LEN(list));
    ASSERT(6 == MYUVAMQP_LIST_LEN(rlist));

    node = list->head;
    ASSERT(0 == memcmp(value1, node->value, strlen(value1)));
    myuvamqp_list_remove_by_value(list, value1);
    node = list->head;
    ASSERT(NULL == node->prev);
    ASSERT(0 == memcmp(value2, node->value, strlen(value2)));
    ASSERT(5 == MYUVAMQP_LIST_LEN(list));
    ASSERT(5 == MYUVAMQP_LIST_LEN(rlist));

    node = list->tail;
    ASSERT(98 == *(int *) node->value);
    myuvamqp_list_remove_by_value(list, &value6);
    node = list->tail;
    ASSERT(NULL == node->next);
    ASSERT(55 == *(int *) node->value);
    ASSERT(4 == MYUVAMQP_LIST_LEN(list));
    ASSERT(4 == MYUVAMQP_LIST_LEN(rlist));

    MYUVAMQP_LIST_FOREACH(list, node)
    {
        i++;
        if(i == 3) break;
    }

    i = 0;
    ASSERT(44 == *(int *) node->value);
    myuvamqp_list_remove_by_value(list, &value4);

    MYUVAMQP_LIST_FOREACH(list, node)
    {
        i++;
        if(i == 3) break;
    }

    i = 0;
    ASSERT(55 == *(int *) node->value);
    ASSERT(3 == MYUVAMQP_LIST_LEN(list));
    ASSERT(3 == MYUVAMQP_LIST_LEN(rlist));

    myuvamqp_list_remove_head(list);
    myuvamqp_list_remove_head(list);

    ASSERT(1 == MYUVAMQP_LIST_LEN(list));
    ASSERT(1 == MYUVAMQP_LIST_LEN(rlist));

    node = list->head;
    ASSERT(55 == *(int *) node->value);
    myuvamqp_list_remove_by_value(list, &value5);
    ASSERT(MYUVAMQP_LIST_EMPTY(list));
    ASSERT(MYUVAMQP_LIST_EMPTY(rlist));

    myuvamqp_list_destroy(list);
}
