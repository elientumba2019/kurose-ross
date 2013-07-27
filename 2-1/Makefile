CC=gcc
CFLAGS=-I. --std=c99 -D_POSIX_C_SOURCE=200112L

lilhttpd: lilhttpd.c
	$(CC) $(CFLAGS) -o lilhttpd lilhttpd.c

clean:
	rm lilhttpd
