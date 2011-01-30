#all: hello.o hello3.o
all: hello_shm.o user_shm

include ../../rtl.mk

user_shm: user_shm.c hello_shm.h
	$(CC) $(INCLUDE) user_shm.c -o user_shm

clean:
	rm -f *.o user_shm

include $(RTL_DIR)/Rules.make
