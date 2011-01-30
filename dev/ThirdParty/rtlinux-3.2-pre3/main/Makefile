#
# Main RTLinux system.
#
# Copyright (C) 1999 FSMLabs
# Written by Cort Dougan <cort@fsmlabs.com>, Victor Yodaiken <yodaiken@fsmlabs.com>
# 
#

all: rtl.o
main: all

include ../rtl.mk

rtl.o: arch/arch.o rtl_core.o rtl_printf.o reserve_cpu.o
	$(LD) -r -o rtl.o arch/arch.o rtl_core.o rtl_printf.o reserve_cpu.o
	cp -f rtl.o ../modules/

clean:
#	rm -f arch
	find . \( -name '*~' -o -name '*.o' -o -name core \) -exec /bin/rm -r '{}' \;

.PHONY: dummy

include $(RTL_DIR)/Rules.make
