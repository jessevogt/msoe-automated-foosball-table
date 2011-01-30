#ifndef __RTL_SYS_MMAN_H__
#define __RTL_SYS_MMAN_H__

#include <linux/mman.h>

extern caddr_t  mmap(void  *start,  size_t length, int prot , int flags, int fd, off_t offset);
extern int munmap(void *start, size_t length);

extern inline int mprotect(const void *addr, size_t len, int prot) { return 0; }

extern inline int msync(const void *start, size_t length, int flags) { return 0; }

extern inline int mlock(const void *addr, size_t len) { return 0; }

extern inline int munlock(const void *addr, size_t len) { return 0; }

extern inline int mlockall(int flags) { return 0; }

extern inline int munlockall(void) { return 0; }

#endif
