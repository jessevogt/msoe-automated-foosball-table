// linux/ioport.h compatibility header

#ifndef _COMPAT_IOPORT_H
#define _COMPAT_IOPORT_H

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,20)

#define check_mem_region(start,n) 0
#define request_mem_region(start,n,name) ((void*) 0)
#define release_mem_region(start,n)

#endif

#include_next <linux/ioport.h>

#endif // _COMPAT_IOPORT_H

