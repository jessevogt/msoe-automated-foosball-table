all: pwm1.o pwm2.o set_pwm

include ../../rtl.mk

set_pwm: set_pwm.c
	$(CC) ${USER_CFLAGS} ${INCLUDE} -Wall -O2 -o set_pwm set_pwm.c
	
clean:
	rm -f *.o set_pwm

include $(RTL_DIR)/Rules.make
