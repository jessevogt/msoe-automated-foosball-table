all: creator.o writer-1.o writer-2.o writer-3.o writer-4.o reader-1.o reader-2.o reader-3.o reader-4.o

include ../../rtl.mk


creator.o: creator.c
	$(CC) ${INCLUDE} ${CFLAGS} -g -c creator.c -o creator.o

reader-1.o: reader-1.c
	$(CC) ${INCLUDE} ${CFLAGS} -g -c reader-1.c -o reader-1.o

reader-2.o: reader-2.c
	$(CC) ${INCLUDE} ${CFLAGS} -g -c reader-2.c -o reader-2.o

reader-3.o: reader-3.c
	$(CC) ${INCLUDE} ${CFLAGS} -g -c reader-3.c -o reader-3.o

reader-4.o: reader-4.c
	$(CC) ${INCLUDE} ${CFLAGS} -g -c reader-4.c -o reader-4.o

writer-1.o: writer-1.c
	$(CC) ${INCLUDE} ${CFLAGS} -g -c writer-1.c -o writer-1.o

writer-2.o: writer-2.c
	$(CC) ${INCLUDE} ${CFLAGS} -g -c writer-2.c -o writer-2.o

writer-3.o: writer-3.c
	$(CC) ${INCLUDE} ${CFLAGS} -g -c writer-3.c -o writer-3.o

writer-4.o: writer-4.c
	$(CC) ${INCLUDE} ${CFLAGS} -g -c writer-4.c -o writer-4.o

clean:
	rm -f *.o *~

include $(RTL_DIR)/Rules.make
