# Makefile for rt_com tests
#
# Copyright (C) 2000 Jochen Küpper


SYSTEM = RTLINUX_V3

CC 			 = gcc

ifeq ($(SYSTEM),RTLINUX_V3)
include ../../../rtl.mk
else
CFLAGS			+= -I. -I.. -O2 -fomit-frame-pointer
endif

OBJS                     = testcom.o

PROGS                    =


.c.o:			Makefile
			$(CC) $(CFLAGS) -c $<


test:			$(OBJS) $(PROGS)


all:
			@cd .. && $(MAKE) all


clean:
			rm -fr $(OBJS) $(PROGS)
