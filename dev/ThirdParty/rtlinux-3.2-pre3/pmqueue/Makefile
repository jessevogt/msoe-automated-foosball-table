all: mqueue

include ../rtl.mk

CFLAGS += -I.

mqueue: rtl_mqueue.o
	cp -f $^ ../modules/

rtl_mqueue.o: mqueue.o mqh.o mq_prio.o
	$(LD) $(LDFLAGS) -r -o rtl_mqueue.o $^

mqueue.o: mqueue.c  mqh.h mq_prio.h

mqh.o: mqh.c mqh.h mq_prio.h

mq_prio.o: mq_prio.c  mq_prio.h

install: rtl_mqueue.o rtl_mqh.o rtl_mq_prio.o
	install -c -m 644 $^ ${RTL_DIR}/modules

clean:
	rm -f *.o

include $(RTL_DIR)/Rules.make
