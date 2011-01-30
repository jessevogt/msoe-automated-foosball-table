all: psc_irq_handler periodic_handler psc_fifo

include ../../rtl.mk

# this is only necessary because we asume that rtlinux is not 
# installed in a system wide location while running tests/examples
PSC_LIBS=-L $(RTL_DIR)/psc/

psc_irq_handler: psc_irq_handler.c
	$(CC) ${USER_CFLAGS} ${INCLUDE} -Wall -O2 ${PSC_LIBS} -o psc_irq_handler psc_irq_handler.c -lrtlinux

periodic_handler: periodic_handler.c
	$(CC) ${USER_CFLAGS} ${INCLUDE} -Wall -O2 ${PSC_LIBS} -o periodic_handler periodic_handler.c -lrtlinux

psc_fifo: psc_fifo.c
	$(CC) ${USER_CFLAGS} -g ${INCLUDE} -Wall -O2 ${PSC_LIBS} -o psc_fifo psc_fifo.c -lrtlinux

clean:
	rm -f *.o psc_irq_handler periodic_handler
