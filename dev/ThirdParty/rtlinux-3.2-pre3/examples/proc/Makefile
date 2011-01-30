KMODS=simple_proc.o proc_start_stop.o sysctl.o sysctl_start_stop.o

all: $(KMODS)

include ../../rtl.mk

start_stop: start_stop.c
	$(CC) -O2 -O2 -Wall start_stop.c -o start_stop

clean:
	rm -f *.o 

include $(RTL_DIR)/Rules.make
