
#define LPT_PORT 0x378
#define LPT_IRQ 7
#define RTC_IRQ 8

#include <asm/rt_time.h>


struct sample {
	RTIME min;
	RTIME max;
};
