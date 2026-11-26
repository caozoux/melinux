CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
LDLIBS = -libverbs -lrdmacm

.PHONY: all clean

all: server client

server: server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

client: client.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f server client
