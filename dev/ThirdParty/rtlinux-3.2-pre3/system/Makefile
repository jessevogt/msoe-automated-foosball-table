# (c) Victor Yodaiken 1998, Released under GPL

all: rtl_posixio.o
# ../modules/rtl_ipc.o

include ../rtl.mk
depend: rtl_sched.o
	$(CC) ${INCLUDE} ${CFLAGS} -M rtl_posixio.c > .depends

rtl_posixio.o: rtl_posixio.c ../include/rtl_posixio.h 
	$(CC) ${INCLUDE}  ${CFLAGS} -o rtl_posixio.o -c rtl_posixio.c
	cp -f rtl_posixio.o ../modules

../modules/rtl_ipc.o: rtl_ipc.o
	cp -f rtl_ipc.o ../modules

install: rtl_posixio.o
	install -c -m 644 rtl_posixio.o ${RTL_DIR}/modules

clean:
	rm -f *.o

include $(RTL_DIR)/Rules.make
