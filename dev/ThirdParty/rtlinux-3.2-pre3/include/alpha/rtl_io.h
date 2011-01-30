#ifndef __RTL_IO_H__
#define __RTL_IO_H__

#include <asm/io.h>

#define rtl_outb(a, b) outb(a, b)
#define rtl_outb_p(a, b) outb(a, b)
#define rtl_inb_p(a) inb(a)
#define rtl_inb(a) inb(a)

#endif
