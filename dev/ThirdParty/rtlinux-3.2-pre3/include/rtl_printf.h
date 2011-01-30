#ifndef __RTL_PRINTF_H__
#define __RTL_PRINTF_H__

extern int rtl_printf(const char * fmt, ...);
#define rt_printk rtl_printf

/* goes directly to console_drivers */
extern int rtl_cprintf(const char * fmt, ...);

#ifdef __KERNEL__
extern int rtl_printf_init(void);
extern void rtl_printf_cleanup(void);
#endif

#endif
