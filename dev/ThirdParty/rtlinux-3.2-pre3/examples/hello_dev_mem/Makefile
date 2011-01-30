all: hello.o write_dev_mem read_dev_mem

include ../../rtl.mk

OBJS = hello_func.o hello_core.o

module: hello.o

hello.o: $(OBJS) hello.h
	ld -r $(OBJS) -o hello.o

write_dev_mem: write_dev_mem.c hello.h
	$(CC) write_dev_mem.c -o write_dev_mem

read_dev_mem: read_dev_mem.c hello.h
	$(CC) read_dev_mem.c -o read_dev_mem

clean:
	rm -f *.o dev_mem read_dev_mem

include $(RTL_DIR)/Rules.make
