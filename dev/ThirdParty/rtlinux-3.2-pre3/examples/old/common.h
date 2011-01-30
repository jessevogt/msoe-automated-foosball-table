
#define LPT_PORT 0x378
#define LPT_IRQ 7
#define RTC_IRQ 8

#include <rtl_time.h>


struct sample {
	struct timespec min;
	struct timespec max;
};
