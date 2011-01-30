all: frank_module.o frank_app rt-handler.o sem_fifohandler.o sig_fifohandler.o fifo_send

include ../../rtl.mk
frank_app: frank_app.c
	$(CC) ${INCLUDE} ${USER_CFLAGS} -O2 -Wall frank_app.c -o frank_app

fifo_send: fifo_send.c
	$(CC) ${INCLUDE} ${USER_CFLAGS} -O2 -Wall fifo_send.c -o fifo_send

frank_module.o: frank_module.c
	$(CC) ${INCLUDE} ${CFLAGS} -c frank_module.c -o frank_module.o 

clean:
	rm -f frank_app *.o

include $(RTL_DIR)/Rules.make
