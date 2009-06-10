lib_LTLIBRARIES = libaochat.la

libaochatdir = ${includedir}

libaochat_la_SOURCES = connect.c event.c keyex.c list.c \
		misc.c packet.c send_queue.c timer.c msg_queue.c
libaochat_la_LDFLAGS = -version-info 2:0:0
libaochat_HEADERS = aochat.h
noinst_HEADERS =

AM_CFLAGS = -g -pipe -O2 -Wall -D__COMPILING_LIB
