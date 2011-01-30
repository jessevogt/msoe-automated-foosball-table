all: posix_fifo.o simple_fifo.o read_fifo fifo_ioctl.o user_fifo_ioctl

include ../../rtl.mk

read_fifo: read_fifo.c
	$(CC) ${USER_CFLAGS} ${INCLUDE} -Wall -O2 -o read_fifo read_fifo.c

user_fifo_ioctl: user_fifo_ioctl.c my_ioctl.h
	$(CC) ${USER_CFLAGS} ${INCLUDE} -Wall -O2 -o user_fifo_ioctl user_fifo_ioctl.c
clean:
	rm -f *.o read_fifo user_fifo_ioctl

include $(RTL_DIR)/Rules.make
