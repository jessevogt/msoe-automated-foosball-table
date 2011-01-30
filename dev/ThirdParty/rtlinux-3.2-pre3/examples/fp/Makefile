all: rt_process.o fptest

include ../../rtl.mk

MYCFLAGS = -O2 -Wall

ifeq ($(ARCH),ppc)
CFLAGS += -mhard-float
endif

rt_process.o: rt_process.c
	$(CC) ${INCLUDE} ${CFLAGS} -c -o rt_process_tmp.o rt_process.c
	$(LD) -r -static rt_process_tmp.o -o rt_process.o -L/usr/lib -lm
	rm -f rt_process_tmp.o

compat.o: compat.c
	$(CC) ${INCLUDE} ${CFLAGS} -c -o compat_tmp.o compat.c
	$(LD) -r -static compat_tmp.o -o compat.o -L/usr/lib -L/usr/lib -lm 
	rm -f compat_tmp.o

fptest: fptest.c
	$(CC) ${INCLUDE} -O2 -Wall -o fptest fptest.c -lm 

test: rt_process.o fptest
	insmod rt_process.o
	./fptest


trigmod.o: trigmod.c
	$(CC) ${INCLUDE} ${CFLAGS} -c trigmod.c -o trigmod_tmp.o
	$(LD) -r -static trigmod_tmp.o -o trigmod.o -L/usr/lib -lm 

trigapp: trigapp.c
	$(CC) ${INCLUDE} -o trigapp trigapp.c 


clean:
	rm -f *.o trigapp fptest sin.data cos.data

include $(RTL_DIR)/Rules.make
