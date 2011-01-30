CFLAGS += -G 0 -mno-abicalls -fno-pic -ffixed-8 -fomit-frame-pointer -pipe -mcpu=r8000 -mips2 -mlong-calls

ifneq (${HOSTARCH},${ARCH})
CROSS_COMPILE	= mips-linux-
USER_CFLAGS	= -static
else
CROSS_COMPILE	=
endif

LIBGCC		= libgcc.o

ifneq ($(wildcard ../../rtl.mk),)
include ../../rtl.mk
endif

libgcc.o: mips/libgcc2.c
	$(CC) $(CFLAGS) -DCROSS_COMPILE -DIN_GCC -I./include -DIN_LIBGCC2 -D__GCC_FLOAT_NOT_NEEDED  -I. -c -DL_muldi3 mips/libgcc2.c -o libgcc.o
