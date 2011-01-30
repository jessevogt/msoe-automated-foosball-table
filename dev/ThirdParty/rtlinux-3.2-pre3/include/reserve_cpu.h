/* Victor Yodaiken 2000.
 * Suspend and restart Linux execution.
 */

#include <linux/config.h>
#include <rtl_conf.h>

#ifndef SUSPEND_LINUX_H
#define SUSPEND_LINUX_H
#ifdef CONFIG_RTL_SUSPEND_LINUX
#ifdef  CONFIG_SMP
int rtlinux_suspend_linux_signal(void);
int rtlinux_restore_linux_signal(void);
int rtlinux_restart_irq(void);
int rtlinux_suspend_irq(void);
void rtlinux_suspend_linux_init(void);
void rtlinux_suspend_linux_cleanup(void);
#ifdef RTL_SIG_SUSPEND_LINUX 
#undef RTL_SIG_SUSPEND_LINUX 
#undef RTL_SIG_RESTART_LINUX 
#endif 
#define RTL_SIG_SUSPEND_LINUX  rtlinux_suspend_linux_signal()
#define RTL_SIG_RESTART_LINUX  rtlinux_restore_linux_signal()
#else /* not SMP */
#define rtlinux_suspend_linux_init() do { ; } while (0)
#define rtlinux_suspend_linux_cleanup() do { ; } while (0)
#endif
#else /* not enabled */
#define rtlinux_suspend_linux_init() do { ; } while (0)
#define rtlinux_suspend_linux_cleanup() do { ; } while (0)
#endif
#endif
