/*
 * RTLinux unistd.h support
 *
 * Written by Michael Barabanov
 * Copyright (C) Finite State Machine Labs Inc., 2000
 * Released under the terms of the GPL Version 2
 *
 */

#ifndef __RTL_UNISTD_H__
#define __RTL_UNISTD_H__

#define _RTL_POSIX_AEP_REALTIME_MINIMAL 1
#define _RTL_POSIX_AEP_REALTIME_LANG_C89 1
/* #define _POSIX_THREADS */

#include <rtl_posixio.h>

extern int open(const char *pathname, int flags);
extern int close(int fd);
extern ssize_t write(int fd, const void *buf, size_t count);
extern ssize_t read(int fd, void *buf, size_t count);
extern off_t lseek(int fildes, off_t offset, int whence);

extern int ioctl(int fd, int request, ...);

#ifdef __KERNEL__

#undef CHILD_MAX
#define CHILD_MAX 0

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

extern long sysconf(int name);

#endif

#endif
