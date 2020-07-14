CC=gcc
INCLUDEDIR = .
override CFLAGS += -g -Wall -Wno-unused-variable -I$(INCLUDEDIR)
LIBNAME = libmyuvamqp
SRCDIR=src
SNAME = $(SRCDIR)/$(LIBNAME).a
DNAME = $(SRCDIR)/S(LIBNAME).so
OBJS = $(SRCDIR)/myuvamqp_buffer.o	\
		$(SRCDIR)/myuvamqp_mem.o	\
		$(SRCDIR)/myuvamqp_list.o	\
		$(SRCDIR)/myuvamqp_utils.o	\
		$(SRCDIR)/myuvamqp_frame.o	\
		$(SRCDIR)/myuvamqp_channel.o	\
		$(SRCDIR)/myuvamqp_connection.o	\
		$(SRCDIR)/myuvamqp_content.o


AR = ar -crs $(SNAME)

all: $(SNAME) $(DNAME)

$(SNAME): $(OBJS)
	$(AR) $(OBJS)

$(SRCDIR)/myuvamqp_buffer.o: $(SRCDIR)/myuvamqp_buffer.c
	gcc -c $(SRCDIR)/myuvamqp_buffer.c -o $(SRCDIR)/myuvamqp_buffer.o $(CFLAGS)

$(SRCDIR)/myuvamqp_mem.o: $(SRCDIR)/myuvamqp_mem.c
	gcc -c $(SRCDIR)/myuvamqp_mem.c -o $(SRCDIR)/myuvamqp_mem.o $(CFLAGS)

$(SRCDIR)/myuvamqp_list.o: $(SRCDIR)/myuvamqp_list.c
	gcc -c $(SRCDIR)/myuvamqp_list.c -o $(SRCDIR)/myuvamqp_list.o $(CFLAGS)

$(SRCDIR)/myuvamqp_utils.o: $(SRCDIR)/myuvamqp_utils.c
	gcc -c $(SRCDIR)/myuvamqp_utils.c -o $(SRCDIR)/myuvamqp_utils.o $(CFLAGS)

$(SRCDIR)/myuvamqp_frame.o: $(SRCDIR)/myuvamqp_frame.c
	gcc -c $(SRCDIR)/myuvamqp_frame.c -o $(SRCDIR)/myuvamqp_frame.o $(CFLAGS)

$(SRCDIR)/myuvamqp_connection.o: $(SRCDIR)/myuvamqp_connection.c
	gcc -c $(SRCDIR)/myuvamqp_connection.c -o $(SRCDIR)/myuvamqp_connection.o $(CFLAGS)

$(SRCDIR)/myuvamqp_channel.o: $(SRCDIR)/myuvamqp_channel.c
	gcc -c $(SRCDIR)/myuvamqp_channel.c -o $(SRCDIR)/myuvamqp_channel.o $(CFLAGS)

$(SRCDIR)/myuvamqp_content.o: $(SRCDIR)/myuvamqp_content.c
	gcc -c $(SRCDIR)/myuvamqp_content.c -o $(SRCDIR)/myuvamqp_content.o $(CFLAGS)
#
# $(SRCDIR)/connection.o: $(SRCDIR)/connection.c
# 	gcc -c $(SRCDIR)/connection.c -o $(SRCDIR)/connection.o $(CFLAGS)
#
# $(SRCDIR)/channel.o: $(SRCDIR)/channel.c
# 	gcc -c $(SRCDIR)/channel.c -o $(SRCDIR)/channel.o $(CFLAGS)
#
# $(SRCDIR)/operation.o: $(SRCDIR)/operation.c
# 	gcc -c $(SRCDIR)/operation.c -o $(SRCDIR)/operation.o $(CFLAGS)
#
# $(SRCDIR)/content.o: $(SRCDIR)/content.c
# 	gcc -c $(SRCDIR)/content.c -o $(SRCDIR)/content.o $(CFLAGS)
#
# $(SRCDIR)/list.o: $(SRCDIR)/list.c
# 	gcc -c $(SRCDIR)/list.c -o $(SRCDIR)/list.o $(CFLAGS)

.PHONY = clean all

clean:
	rm -rf $(SRCDIR)/*.a
	rm -rf $(SRCDIR)/*.so
	rm -rf $(SRCDIR)/*.o
