/*
 * rtlcpp.h - C++ support for RTL
 * Michael Barabanov, <baraban@fsmlabs.com>
 *
 * Ideas and code from David Olofson, Yunho Jeon and myself
 */

#ifndef __KERNELCPP__
#define __KERNELCPP__

#ifdef __cplusplus
extern "C" {
#endif

#define new _new
#define virtual _virtual
#define NULL 0
#include <linux/kernel.h>
#include <linux/module.h>
#include <rtl_sched.h>
#include <rtl_fifo.h>
#include <rtl_time.h>
#include <pthread.h>

#undef new
#undef virtual
#ifdef __cplusplus
}
#endif

extern "C" {

void *kmalloc(unsigned size, int prio);
void kfree(const void *p);
};

extern "C" {
	int __do_global_ctors_aux();
	int __do_global_dtors_aux();
}

void *operator new (unsigned size) {
  return kmalloc(size,0);
}

void *operator new[] (unsigned size) {
  return kmalloc(size,0);
}

void operator delete (void *p) {
  kfree(p);
}

void operator delete[] (void *p) {
  kfree(p);
}

extern "C" {
	int init_module();
	void cleanup_module();
}

#endif
