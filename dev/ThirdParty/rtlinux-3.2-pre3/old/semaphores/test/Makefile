TARGET = test_ipc.o

SRC = $(TARGET:.o=.c)

all:		$(TARGET)

# you might have to change this
ifneq ($(wildcard ../../rtl.mk),)
include ../../rtl.mk
endif

# the path to the rt-linux kernel 

$(TARGET):	$(SRC)
	$(CC)  ${INCLUDE} ${CFLAGS} -D__KERNEL__ -D__RT__  -c $(SRC)

ins:		$(TARGET)
	insmod $(TARGET)

rm:
	rmmod $(TARGET)

rtl:
	insmod ${RTL_DIR}/modules/rtl_sched
	insmod ${RTL_DIR}/modules/rtl_fifo

rt_ipc:
	insmod ../rt_ipc.o

clean:
	rm -f $(TARGET)

include $(RTL_DIR)/Rules.make
