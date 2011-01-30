all: rt_irq_gen.o rectangle.o lpt_rt_irq.o rectangle.o rt_irq_gen.o sched_toggle.o lpt_irq read_lpt

include ../../rtl.mk

lpt_irq: lpt_irq.c
	$(CC) ${INCLUDE} -Wall -O2 -o lpt_irq lpt_irq.c

read_lpt: read_lpt.c
	$(CC) ${INCLUDE} -Wall -O2 -o read_lpt read_lpt.c

clean:
	rm -f read_lpt lpt_irq  *.o
