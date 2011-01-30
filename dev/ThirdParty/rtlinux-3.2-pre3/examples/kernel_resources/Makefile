KMODS=tasklet.o tasklet_cleanup.o rt_kthread.o rt_allocator.o \
	kthread1.o rt_sig_mmap2.o rt_sig_thread.o kthread_sig.o simple_device.o \
	mmap_kernel_user.o rt_sig_mmap1.o kcp.o kmore.o alloc_syscall.o

UMODS=read_simple_device uthread1 mmap_user1 mmap_user2 read_rt_kthread syscall_check

.PHONY: all
all: $(KMODS) $(UMODS)

syscall: syscall.c
	$(CC) syscall.c -o syscall

syscall_check: syscall_check.c
	$(CC) syscall_check.c -o syscall_check

uthread1: uthread1.c
	$(CC) uthread1.c -o uthread1

read_simple_device: read_simple_device.c
	$(CC) read_simple_device.c -o read_simple_device

read_rt_kthread: read_rt_kthread.c
	$(CC) read_rt_kthread.c -o read_rt_kthread

mmap_user1: mmap_user1.c mmap_user2.c device_common.h
	$(CC) mmap_user1.c -o mmap_user1

mmap_user2: mmap_user1.c mmap_user2.c device_common.h
	$(CC) mmap_user2.c -o mmap_user2

include ../../rtl.mk

.PHONY: clean
clean:
	rm -f *.o $(UMODS)
