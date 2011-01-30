#ifndef __RTL_SYS_UTSNAME_H__
#define __RTL_SYS_UTSNAME_H__

#ifdef __KERNEL__
#include <linux/utsname.h>
#define utsname new_utsname

static inline int uname(struct new_utsname * name)
{

	*name = system_utsname;
	return 0;
}
#endif

#endif
