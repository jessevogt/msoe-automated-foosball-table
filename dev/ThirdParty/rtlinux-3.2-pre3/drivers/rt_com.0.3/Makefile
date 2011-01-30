# Standalone Makefile for rt_com
#
# Copyright (C) 1997-1999 Jochen Küpper


# CFLAGS		+= -c -I/usr/include/rtlinux -O2
include ../../rtl.mk
# CFLAGS		+= -DRTLINUX_V1

INSTALL 	= install -c


all:		rt_com.o

tests: testcom.o com_posix.o


install:	rt_com.o
	$(INSTALL) rt_com.o /lib/modules/`uname -r`/misc/rt_com.o


rt_com.o:	rt_com.c rt_com.h rt_comP.h
	$(CC) $(CFLAGS) -c rt_com.c

clean:
	rm -f *.o
