# (c) Victor Yodaiken 1998, Released under GPL

all: rtl_time.o rtl_timer.o rtl_sched.o 

clean:
	rm -f *.o

include ../rtl.mk

# either this conditional is needed, or arch/Makefile needs to be split up
#  into cflags and libgcc stuff -- Michael

ifneq ($(ARCH),mips)
LIBGCC := $(shell $(CC) -print-libgcc-file-name)
else
LIBGCC		= libgcc.o
libgcc.o: mips/libgcc2.c
	$(CC) $(CFLAGS) -DCROSS_COMPILE -DIN_GCC -I./include -DIN_LIBGCC2 -D__GCC_FLOAT_NOT_NEEDED  -I. -c -DL_muldi3 mips/libgcc2.c -o libgcc.o
endif

LDFLAGS=-d

depend: rtl_sched.o
	$(CC) ${INCLUDE} ${CFLAGS} -M rtl_sched.c > .depends

rtl_sched.o: rtl_sched_tmp.o switch.o sw.o rtl_mutex.o unistd.o signal.o rtl_sema.o rtl_posix.o rtl_timer.o rtl_barrier.o $(LIBGCC)
	$(LD) $(LDFLAGS) -r -o rtl_sched.o rtl_sched_tmp.o switch.o sw.o rtl_mutex.o unistd.o signal.o rtl_sema.o rtl_posix.o rtl_timer.o rtl_barrier.o  -static ${LIBGCC}
	cp -f rtl_sched.o ../modules/

rtl_sched_tmp.o: rtl_sched.c ../include/rtl_sched.h 
	$(CC) ${INCLUDE}  ${CFLAGS} -o rtl_sched_tmp.o -c rtl_sched.c

rtl_compat.o: rtl_compat.c ../include/rtl_sched.h 
	$(CC) ${INCLUDE}  ${CFLAGS} -o rtl_compat.o -c rtl_compat.c

switch.o: $(ARCH)/switch.c ../include/rtl_sched.h
	$(CC) ${INCLUDE}  ${CFLAGS} -o switch.o -c $(ARCH)/switch.c

sw.o: $(ARCH)/sw.S ../include/rtl_sched.h
	$(CC) ${INCLUDE} ${CFLAGS} -D__ASSEMBLY__ -o sw.o -c $(ARCH)/sw.S

rtl_time_tmp.o: $(ARCH)/rtl_time.c ../include/arch/rtl_time.h ../include/rtl_time.h
	$(CC) ${INCLUDE}  ${CFLAGS} -c -o  $@ $<

rtl_time_common.o: rtl_time_common.c ../include/arch/rtl_time.h ../include/rtl_time.h
	$(CC) ${INCLUDE}  ${CFLAGS} -c -o  $@ $<

rtl_time.o:  rtl_time_tmp.o rtl_time_common.o
	$(LD) -r -o rtl_time.o rtl_time_tmp.o rtl_time_common.o $(LIBGCC)
	cp -f rtl_time.o ../modules

rtl_timer.o: rtl_timer.c ../include/rtl_timer.h
	$(CC) ${INCLUDE}  ${CFLAGS} -o rtl_timer.o -c rtl_timer.c

install: rtl_sched.o
	install -c -m 644 rtl_sched.o ${RTL_DIR}/modules

include $(RTL_DIR)/Rules.make
