all: rtc_test.o rtc_fifo_test.o multitask.o
# testresched.o sendresched.o
# all: rt_process.o rt_irq.o rt_irq_gen.o nonrt_irq.o monitor

MYCFLAGS = -O2 -Wall
include ../../rtl.mk

periodic_monitor: periodic_monitor.c
	$(CC) ${INCLUDE} ${MYCFLAGS} -o periodic_monitor periodic_monitor.c

oneshot_test.o: oneshot_test.c common.h
	$(CC) ${INCLUDE} ${CFLAGS} -c oneshot_test.c

multitask.o: multitask.c
	$(CC) ${INCLUDE} ${CFLAGS} -c multitask.c

periodic_test.o: periodic_test.c common.h
	$(CC) ${INCLUDE} ${CFLAGS} -c periodic_test.c

rtc_test.o: rtc_test.c common.h
	$(CC) ${INCLUDE} ${CFLAGS} -c rtc_test.c

rtc_fifo_test.o: rtc_fifo_test.c common.h
	$(CC) ${INCLUDE} ${CFLAGS} -c rtc_fifo_test.c

sched_test.o: sched_test.c common.h
	$(CC) ${INCLUDE} ${CFLAGS} -c sched_test.c

clean:
	rm -f *.o monitor 

include $(RTL_DIR)/Rules.make
