include ../../rtl.mk

ifneq ($(wildcard $(RTL_DIR)/.config),)
include $(RTL_DIR)/.config
endif

EX = nanosleep.o
EX += create_recursive.o print_mod.o print_app
EX += posix_mod.o posix_app

ifdef CONFIG_RTL_SUSPEND_LINUX
EX += reserve_cpu.o
endif

ifdef CONFIG_RTL_CLOCK_GPOS
EX += clock_gpos.o
endif

ifdef CONFIG_RTFPREALLOC
EX += rtf_prealloc.o
endif

all: $(EX) 

print_app: print_app.c
	$(CC) ${USER_CFLAGS} -o print_app print_app.c

posix_app: posix_app.c
	$(CC) ${USER_CFLAGS} -o posix_app posix_app.c

clean:
	rm -f *.o $(EX)

include $(RTL_DIR)/Rules.make
