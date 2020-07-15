#ident "$Id: Makefile,v 1.0 2020/03/31 $"
## -----------------------------------------------------------------------
##
##   Copyright 2020 Aliyun Corporation - All Rights Reserved
##
##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as published by
##   the Free Software Foundation, Inc..
##
## -----------------------------------------------------------------------

#
# Makefile for sysreg
#

CC       = gcc -Wall
CFLAGS   = -g -O0 -fomit-frame-pointer -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -I ./
LDFLAGS  =
BIN	= rsysreg wsysreg
sbindir = /usr/sbin

RSRC = rsysreg.c
RSRC += aarch64_rdmsr.c
WSRC = wsysreg.c
WSRC += aarch64_wrmsr.c


#OBJS = $(SRC:%.c=%.o)

#$(BIN): $(OBJS)


all:
	$(CC) $(CFLAGS) $(RSRC) -o rsysreg
	$(CC) $(CFLAGS) $(WSRC) -o wsysreg

clean:
	rm -f *.o $(BIN)

distclean: clean
	rm -f *~ \#*

install: all
	cp regsv8_0.table ~/
	install -m 755 $(BIN) $(sbindir)
	echo "sysreg-tools have installed successfully!"
test:
	make
