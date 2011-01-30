all: rtl_tracer.o tracer rt_process.o monitor

include rtl.mk

tracer: tracer.c tracedump.h
	$(CC) $(INCLUDE) -O2 -Wall -o tracer tracer.c

rtl_tracer.o: rtl_tracer.c
	$(CC) -c $(CFLAGS) -ortl_tracer.o  rtl_tracer.c

monitor: monitor.c
	$(CC) ${INCLUDE}  -Wall -O2 -o monitor monitor.c

clean:
	rm -f tracer *.o monitor

test: all
	-rmmod rt_process
	-rmmod rtl_tracer
	-rmmod mbuff
	-insmod $(RTL_DIR)/drivers/mbuff/mbuff.o
	insmod rtl_tracer.o
	insmod rt_process.o
	./tracer

