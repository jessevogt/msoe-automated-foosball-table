/*
 * (C) Finite State Machine Labs Inc. 1997 business@fsmlabs.com
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

#define LPT_PORT 0x378
#define LPT_IRQ 7
#define RTC_IRQ 8

#include <rtl_time.h>


struct sample {
	hrtime_t min;
	hrtime_t max;
};
