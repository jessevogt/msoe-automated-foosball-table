CC        = $(CROSS_COMPILE)gcc
CPP       = $(CROSS_COMPILE)gcc -E

LDFLAGS   = 

include ../../rtl.mk
CFLAGS += -I.


# Remove -DSHM_DEMO for real use. 
# Remove -D__RT__ for non real time Linux

# -DMBUFF_LIB_ONLY disables registration of its own misc char device by mbuff.o
# this is for use as shm library only for RT-FIFOs

.SUFFIXES: .o .c .h .m .i .S


OBJS = mmap.o kvmem.o mbuff_proc.o

all: module
#	@echo $(LINUXDIR) assumed to be your kernel source directory.
	@echo "do 'make tests' to build all mbuff demo programs."

tests: tester mbuff mbuff_alloc mbuff_dealloc demo

module: mbuff.o #rtl_nfifo.o

mbuff: 
	-mknod ./mbuff c 10 254

mbuff.o: $(OBJS)
	$(CROSS_COMPILE)ld -r $(OBJS) -o mbuff.o
	cp -f mbuff.o $(RTL_DIR)/modules

clean:
	-rm -f $(OBJS) *.o tester core mbuff_alloc mbuff_dealloc mbuff demo

#.m.o:
#	$(CC) $(INCLUDES) -c $(INCDIRS) $(CFLAGS) $(X_CFLAGS) $(DEBUGFLAGS) $*.m
#.c.o:
#	$(CC) $(INCLUDES) -c $(INCDIRS) $(CFLAGS) $(X_CFLAGS) $(DEBUGFLAGS) $*.c -o $@
#.c.s:
#	$(CC) $(INCLUDES) -S $(INCDIRS) $(CFLAGS) $(X_CFLAGS) $(DEBUGFLAGS) $*.c -o $@
#.c.i:
#	$(CC) $(INCLUDES) -E $(INCDIRS) $(CFLAGS) $(X_CFLAGS) $(DEBUGFLAGS) $*.c > $@
#.S.o:
#	$(CC) $(INCLUDES) -c $(INCDIRS) $(CFLAGS) $(X_CFLAGS) $(DEBUGFLAGS) $*.S -o $@
#.s.o:
#	$(CC) $(INCLUDES) -c $(INCDIRS) $(CFLAGS) $(X_CFLAGS) $(DEBUGFLAGS) $*.s -o $@

tester: tester.c mbuff.h
	gcc -g -O2 -DSHM_DEMO -o tester tester.c
mbuff_alloc: mbuff_alloc.c mbuff.h
	gcc -g -O2 -DSHM_DEMO -o mbuff_alloc mbuff_alloc.c
mbuff_dealloc: mbuff_dealloc.c mbuff.h
	gcc -g -O2 -DSHM_DEMO -o mbuff_dealloc mbuff_dealloc.c
demo: demo.c mbuff.h
	gcc -g -O2 -DSHM_DEMO -o demo demo.c

test: test1 test2
test1: tests all
	-rmmod mbuff
	-insmod -f mbuff.o 
	./mbuff_alloc default 409600
# RT-FIFO-000 is the default area when /dev/rtf0 is open
	-./tester

test2: tests all
	-rmmod mbuff
	-insmod -f mbuff.o
	(./tester &); sleep 2; (./tester &); ./mbuff_dealloc default

test1nfifo: tests all
	-rmmod rtl_nfifo
	-rmmod mbuff
	-rmmod rtl_posixio
	-insmod mbuff.o 
	-modprobe rtl_posixio
	-insmod rtl_nfifo.o
	./mbuff_alloc RT-FIFO-000 409600
# RT-FIFO-000 is the default area when /dev/rtf0 is open
	-./tester

test2nfifo: tests all
	-rmmod rtl_nfifo
	-rmmod mbuff
	-rmmod rtl_posixio
	-insmod mbuff.o
	-modprobe rtl_posixio
	-insmod rtl_nfifo.o; (./tester &); sleep 2; (./tester &); ./mbuff_dealloc RT-FIFO-000

include $(RTL_DIR)/Rules.make
