all: sound.o irqsig.o irqthread.o
# testresched.o sendresched.o
# all: rt_process.o rt_irq.o rt_irq_gen.o nonrt_irq.o monitor

MYCFLAGS = -O2 -Wall
include ../../rtl.mk

sound.o: sound.c
	$(CC) ${INCLUDE} ${CFLAGS} -c sound.c

#test, remove any modules, load new ones and run app
test: all
	@echo "Sound test"
	@echo "Type <return> to continue"
	@read junk
	@echo "First we remove any existing rtl-modules"
	@echo "You may see error warnings from \"make\" - ignore them"
	@echo "Type <return> to continue"
	@read junk
	-rmmod hello
	-rmmod sound
	-rmmod rt_process
	-rmmod frank_module
	(cd ../../; scripts/rmrtl)
	@echo "Now insert the fifo and scheduler"
	@echo "Type <return> to continue"
	@read junk
	(cd ../../; scripts/insrtl)
	@echo "Now start the real-time tasks  module"
	@echo "Type <return> to continue"
	@read junk
	@insmod sound.o
	@echo "Now starting the application"
	@echo "Hello, this is Linus Torvalds; I'd like to pronounce \"Linux\": that's Linux!"
	@cat linux.au >/dev/rtf3

clean:
	rm -f *.o

include $(RTL_DIR)/Rules.make
