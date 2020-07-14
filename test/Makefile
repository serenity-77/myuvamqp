CC = gcc
INCLUDEDIR = ../src
LIBDIR = ../src
LIBPATH = ../src/libmyuvamqp.a
CFLAGS = -g -Wall -I$(INCLUDEDIR)

SOURCES := $(filter-out utils.c, $(wildcard *.c))
EXECUTABLES = $(basename $(SOURCES))
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes
DEPS = utils.o

all: deps test

deps: $(DEPS)
	$(CC) -c $(CFLAGS) utils.c -o utils.o -luv

test_buffer: test_buffer.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_list: test_list.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_start_connection: test_start_connection.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_open_channel: test_open_channel.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_queue_declare_delete: test_queue_declare_delete.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_exchange_declare_delete: test_exchange_declare_delete.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_connection_close_by_client: test_connection_close_by_client.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_channel_close: test_channel_close.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_reopen_channel: test_reopen_channel.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_channel_exception: test_channel_exception.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_queue_bind_unbind: test_queue_bind_unbind.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_queue_purge: test_queue_purge.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_basic_qos: test_basic_qos.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_basic_consume: test_basic_consume.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_basic_cancel: test_basic_consume.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_confirm_select: test_confirm_select.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_basic_publish: test_basic_publish.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv

test_content: test_content.c
	$(CC) $(CFLAGS) $^ $(DEPS) -o $@ $(LIBPATH) -luv


test: $(EXECUTABLES)
	for executable in $^; do \
		$(VALGRIND) ./$${executable}; \
	done

.PHONY: clean all test deps

clean:
	rm -rf $(EXECUTABLES)
	rm -rf $(DEPS)
