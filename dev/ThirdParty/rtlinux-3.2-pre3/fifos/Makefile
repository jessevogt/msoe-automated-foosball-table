all: fifo

include ../rtl.mk

fifo: rtl_fifo.o
# rtl_nfifo.o
	cp -f rtl_fifo.o ../modules/
#	cp -f rtl_nfifo.o ../modules/


install: rtl_fifo.o rtl_nfifo.o
	install -c -m 644 rtl_fifo.o rtl_nfifo.o ${RTL_DIR}/modules

clean:
	rm -f rtl_fifo.o rtl_nfifo.o

include $(RTL_DIR)/Rules.make
