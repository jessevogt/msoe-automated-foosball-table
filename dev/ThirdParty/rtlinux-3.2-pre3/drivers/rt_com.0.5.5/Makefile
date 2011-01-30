# Standalone Makefile for rt_com
#
# Copyright (C) 1997-2000 Jochen Küpper
# RTLINUX_V3 modifications - 6 May 2002 Marc Le Douarain

# set here RTAI, RTLINUX_V1, RTLINUX_V2 or RTLINUX_V3
# in accordance with your system
#SYSTEM			= RTAI
SYSTEM			= RTLINUX_V3

DISTFILES		= rt_com.c rt_com_posix.c rt_com.h rt_comP.h \
    ChangeLog COPYING License Makefile README TODO \
    doc/rt_com.tex doc/rt_com.pdf \
    test/Makefile test/hello_world.c test/hello_world2.c test/testcom.c test/testcomrtai.c \
    webpages/index.html

INCLUDEDIR		= ../../include/

INSTALL 		= install -c -m 644

OBJS			= rt_com.o # rt_com_posix.o currently broken ;-(

VERSION			= 0.5.5pre1

ifeq ($(SYSTEM),RTLINUX_V3)
include ../../rtl.mk
endif

CFLAGS			+= -D$(SYSTEM) -DVERSION=\"$(VERSION)\"
ifneq ($(SYSTEM),RTLINUX_V3)
CFLAGS			+= -I../../include -O2
endif

.c.o:			Makefile
			$(CC) $(CFLAGS) -c $<


all:			$(OBJS) examples


doc:			doc/rt_com.pdf


clean:
			rm -fr $(OBJS) testcom.o rt_com-$(VERSION)
			cd test && make clean

dist:			rt_com.tar.gz


install:		rt_com.o
			$(INSTALL) $(OBJS) /lib/modules/`uname -r`/misc/
			$(INSTALL) rt_com.h $(INCLUDEDIR)/rt_com.h


rt_com.tar.gz:		$(DISTFILES)
			@mkdir -p rt_com-$(VERSION)  \
			&& cp -aP $(DISTFILES) rt_com-$(VERSION) \
			&& GZIP=--best tar czf rt_com-$(VERSION).tar.gz rt_com-$(VERSION) \
			&& rm -rf rt_com-$(VERSION)

doc/rt_com.pdf:		doc/rt_com.tex
			@cd doc && pdflatex rt_com && pdflatex rt_com

ifneq ($(SYSTEM),RTLINUX_V3)
examples:
			cd test && make
else
examples:
			cd test && make -f Makefile_rtlinux
test:			$(OBJS) examples
			cd test && make test -f Makefile_rtlinux
endif

