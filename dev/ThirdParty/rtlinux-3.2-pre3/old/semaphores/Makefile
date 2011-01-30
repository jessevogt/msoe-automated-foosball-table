TARGET = rt_ipc.o

ifneq ($(wildcard ../rtl.mk),)
include ../rtl.mk
endif

SRC = $(TARGET:.o=.c)
HDR = $(TARGET:.o=.h)

all:		$(TARGET)


$(TARGET):	$(SRC) $(HDR)
	$(CC) ${INCLUDE} ${CFLAGS} -D__KERNEL__ -D__RT__  -c $(SRC)

ins:		$(TARGET)
	insmod $(TARGET)

rm:
	rmmod $(TARGET)

rtl:
	insmod ${RTL_DIR}/modules/rtl_sched
	insmod ${RTL_DIR}/modules/rt_fifo

clean:
	rm -f $(TARGET)
	$(MAKE) -C test clean; \

include $(RTL_DIR)/Rules.make
