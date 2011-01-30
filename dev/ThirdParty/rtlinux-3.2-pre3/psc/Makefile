# (C) Finite State Machine Labs Inc. 2000 business@fsmlabs.com
#
# Released under the terms of GPL 2.
# Open RTLinux makes use of a patented process described in
# US Patent 5,995,745. Use of this process is governed
# by the Open RTLinux Patent License which can be obtained from
# www.fsmlabs.com/PATENT or by sending email to
# licensequestions@fsmlabs.com

all: archlink psc.o librtlinux.a librtlinuxpSOS.a

ifneq ($(wildcard ../rtl.mk),)
include ../rtl.mk
endif

psc.o: psc.c
	$(CC) ${INCLUDE}  ${CFLAGS} -c psc.c
	cp -f psc.o ../modules

archlink:
	rm -f arch
	ln -sf $(ARCH) arch

rtlinux_signal.o: rtlinux_signal.c
	$(CC) ${INCLUDE} ${USER_CFLAGS} -c rtlinux_signal.c

librtlinux.a: rtlinux_signal.o
	$(LD) -r -o $*.o $?
	rm -f $@
	$(AR) rcs $@ $*.o

rtlinux_psos.o: rtlinux_psos.c
	$(CC) ${INCLUDE} ${USER_CFLAGS} -c rtlinux_psos.c

librtlinuxpSOS.a: rtlinux_psos.o
	$(LD) -r -o $*.o $?
	rm -f $@
	$(AR) rcs $@ $*.o

clean: dummy
	rm -f *.o monitor gnuplot.out modules.dep *.a

.PHONY: dummy

include $(RTL_DIR)/Rules.make
