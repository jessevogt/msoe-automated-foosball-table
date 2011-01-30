all: archlink hello.o rtl_debug.o 

ifneq ($(wildcard ../rtl.mk),)
include ../rtl.mk
endif

ifneq ($(wildcard .depend),)
include .depend
endif

rtl_debug.o: rtl_debug.c arch/rtl-stub.c

archlink:
	rm -f arch
	ln -sf $(ARCH) arch

clean:
	rm -f *.o 

include $(RTL_DIR)/Rules.make
