#ifndef __RTL_ERRNO_H__
#define __RTL_ERRNO_H__

#include <rtl_time.h>
#include <linux/errno.h>

#define ENOTSUP EOPNOTSUPP
#define __set_errno(x) do { errno = (x); } while (0)
#define __reterror(x, y) do { errno = (x); return y; } while (0)

#endif
